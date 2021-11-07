/*
* @author: Inan Evin
* @website: www.inanevin.com
* @twitter: lineupthesky
* @github: https://github.com/inanevin/UE-DataTable-Based-Event-Manager
* 
* Simple Event Manager, exposed to UE4 Editor via DataTables.
*
* MIT License
* Copyright (c) 2021 Inan Evin
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
* 
*/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Delegates/DelegateCombinations.h"
#include "UObject/NoExportTypes.h"
#include "GameEventManager.generated.h"

class UDataTable;

/**
* Event argument types, used to define arguments in Data table.
*/
UENUM(BlueprintType, Blueprintable)
enum class EEventArgTypes : uint8
{
	Int,
	Float,
	Bool,
	FName,
	FString,
	FVector,
	FVector2D,
	FRotator,
	UObjectPtr,
	AActorPtr,
	UEnum,
	CustomStruct
};

/**
* Struct interface to enable passing custom struct pointers through events.
* Whenever you want to use a custom struct as an event parameter, make sure it derives from public FEventArgStruct
*/
USTRUCT(BlueprintType, Blueprintable)
struct FEventArgStruct
{
	GENERATED_BODY()
};

/**
* Allowed base types.
*/
typedef TVariant<int, float, double, FString, FName, bool, FVector, FVector2D, FRotator, UObject*, AActor*, uint8, FEventArgStruct*> EventArgType;

/**
* Wrapper for an event argument. Defines the name of the argument as well as it's type.
*/
class CEventArg
{
public:
	CEventArg()
	{
	}

	CEventArg(const FName& name, EventArgType type) : m_name(name), m_type(type)
	{
	};

	FName m_name = "";
	EventArgType m_type;
};

/**
* Actual row definition for the data table.
*/
USTRUCT(BlueprintType, Blueprintable)
struct FEventDefinition : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Is Dynamic?"))
	bool m_isDynamic = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Event Args"))
	TMap<FName, EEventArgTypes> m_args;
};

/**
* All events use the same delegate structure.
*/
class UGameEvent;
DECLARE_MULTICAST_DELEGATE_OneParam(FGameEventDelegate, UGameEvent&);

/**
* The actual parameter type passed through game events.
* This type contains a list of arguments, that are set according to the DataTable.
* We use Broadcast() method to broadcast a template parameter pack as the arguments.
* If the system detects the order and the type of arguments do not match to the ones defined in DataTable
* It logs an error, and does not broadcast the actual delegate.
*/
UCLASS(BlueprintType, Blueprintable)
class UGameEvent : public UObject
{
	GENERATED_BODY()

public:
	UGameEvent()
	{
	};

	virtual ~UGameEvent()
	{
	};

	void Broadcast()
	{
		BroadcastDelegate();
	}

	FORCEINLINE FGameEventDelegate& GetDelegate() { return m_delegate; }

	template <typename T, typename ... Args>
	void Broadcast(T t, Args ... args)
	{
		SetValues(t, args...);

		if (m_argsCounter == -1 || m_argsCounter < m_eventArgs.Num())
		{
			UE_LOG(LogTemp, Error, TEXT("Broadcast of Event '%s' failed. Args Counter: %d, Event Args Num: %d"),
			       *m_name.ToString(), m_argsCounter, m_eventArgs.Num());
		}

		m_argsCounter = 0;
	}

	template <typename T, typename ... Args>
	void SetValues(T t, Args ... args)
	{
		if (m_argsCounter != -1)
		{
			SetValues(t);
			SetValues(args...);
		}
	}

	/// Actual method where we set the individual params inside the sent param pack to Broadcast()
	template <typename T>
	void SetValues(T arg)
	{
		if (m_argsCounter == -1)
			return;

		// Create a temporary type list, set it's type to T.
		EventArgType tempType;
		tempType.Set<T>(arg);

		// Get the actual set type in the current argument we are supposed to set.
		// If the type indices don't match, abort.
		CEventArg& eventArg = m_eventArgs[m_argsCounter];
		if (eventArg.m_type.GetIndex() == tempType.GetIndex())
		{
			eventArg.m_type.Set<T>(arg);
			CheckArgsCounter();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Broadcast mismatch found in event '%s' between the sent argument and '%s'"),
			       *m_name.ToString(), *eventArg.m_name.ToString());
			m_argsCounter = -1;
		}
	}

	template <typename T>
	T GetValue(const FName& id)
	{
		CEventArg* found = m_eventArgs.FindByPredicate([id](const CEventArg& arg) { return arg.m_name == id; });

		if (found == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("Variable '%s' could not be found in event '%s'"), *id.ToString(), *m_name.ToString());
			return T();
		}

		// Create a temporary type list, set it's type to T.
		EventArgType tempType;
		tempType.Set<T>(T());
		
		if (found->m_type.GetIndex() == tempType.GetIndex())
		{
			return found->m_type.Get<T>();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Requested variable '%s' in event '%s' with the wrong type!"), *id.ToString(), *m_name.ToString());
			return T();
		}
		
	}

	UFUNCTION(BlueprintCallable)
	int GetInt(FName id)
	{
		return GetValue<int>(id);
	}

	UFUNCTION(BlueprintCallable)
	float GetFloat(FName id)
	{
		return GetValue<float>(id);
	}


	UFUNCTION(BlueprintCallable)
	bool GetBool(FName id)
	{
		return GetValue<bool>(id);
	}

	UFUNCTION(BlueprintCallable)
	FVector GetFVector(FName id)
	{
		return GetValue<FVector>(id);
	}

	UFUNCTION(BlueprintCallable)
	FVector2D GetFVector2D(FName id)
	{
		return GetValue<FVector2D>(id);
	}

	UFUNCTION(BlueprintCallable)
	FRotator GetFRotator(FName id)
	{
		return GetValue<FRotator>(id);
	}

	UFUNCTION(BlueprintCallable)
	UObject* GetUObject(FName id)
	{
		return GetValue<UObject*>(id);
	}

	UFUNCTION(BlueprintCallable)
	AActor* GetActor(FName id)
	{
		return GetValue<AActor*>(id);
	}

	FEventArgStruct* GetStruct(FName id)
	{
		return GetValue<FEventArgStruct*>(id);
	}

	UFUNCTION(BlueprintCallable)
	uint8 GetUEnum(FName id)
	{
		return GetValue<uint8>(id);
	}

protected:
	virtual void BroadcastDelegate();

private:
	void CheckArgsCounter();

private:
	friend class UGameEventManager;
	TArray<CEventArg> m_eventArgs;
	FGameEventDelegate m_delegate;
	int m_argsCounter = 0;
	FName m_name = "";
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameEventDelegateDynamic, UGameEvent*, EventData);

/**
* Same as UGameEvent, however this sub-class casts a dynamic delegate instead of a non-dynamic.
* Use only for the events that you want to listen through blueprints, instead of C++ code. ( you can also listen these in C++ )
*/
UCLASS(BlueprintType, Blueprintable)
class UGameEventDynamic : public UGameEvent
{
	GENERATED_BODY()

public:
	UGameEventDynamic()
	{
	};

	virtual ~UGameEventDynamic() override
	{
	};

	FORCEINLINE FGameEventDelegateDynamic& GetDynDelegate() { return m_dynamicDelegate; }

	UPROPERTY(BlueprintAssignable)
	FGameEventDelegateDynamic m_dynamicDelegate;

protected:
	virtual void BroadcastDelegate() override;
};


/**
 * Handles the initialization & management of events.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTSNIPER_API UGameEventManager : public UObject
{
	GENERATED_BODY()

public:
	
	void Setup();
	void Clear();

	UFUNCTION(BlueprintCallable)
	UGameEventDynamic* GetDynamic(FName id, bool& success);

	UGameEvent& Get(const FName& id);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	UDataTable* m_eventDefinitions = nullptr;

	UPROPERTY()
	TMap<FName, UGameEvent*> m_events;
};
