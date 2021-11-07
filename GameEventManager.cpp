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

#include "Core/GameEventManager.h"

void UGameEventManager::Setup()
{
	// List of row names in the datable.
	TArray<FName> rowNames = m_eventDefinitions->GetRowNames();

	// Iterate each row.
	for (const FName& name : rowNames)
	{
		FString contextString;
		FEventDefinition* row = m_eventDefinitions->FindRow<FEventDefinition>(name, contextString);
		if (row)
		{

			// Create a game event for each row, either a normal one or a dynamic one.
			UGameEvent* ev = nullptr;
			if (row->m_isDynamic)
				ev = NewObject<UGameEventDynamic>(GetOuter(), UGameEventDynamic::StaticClass());
			else
				ev = NewObject<UGameEvent>(GetOuter(), UGameEvent::StaticClass());
			ev->m_name = name;
			m_events.Add(name, ev);

			// Iterate all arguments in the row.
			// For each argument, add a new EventArgType type list to the event arguments array.
			// These type lists will determine what is the type of the argument for the event, based on the enumeration value set in editor.
			for (auto& arg : row->m_args)
			{
				EventArgType argType;
				
				if (arg.Value == EEventArgTypes::Int)
					argType.Set<int>(0);
				else if (arg.Value == EEventArgTypes::Float)
					argType.Set<float>(0);
				else if (arg.Value == EEventArgTypes::Bool)
					argType.Set<bool>(0);
				else if (arg.Value == EEventArgTypes::FName)
					argType.Set<FName>("");
				else if (arg.Value == EEventArgTypes::FString)
					argType.Set<FString>("");
				else if (arg.Value == EEventArgTypes::FVector)
					argType.Set<FVector>(FVector::ZeroVector);
				else if (arg.Value == EEventArgTypes::FVector2D)
					argType.Set<FVector2D>(FVector2D::ZeroVector);
				else if (arg.Value == EEventArgTypes::FRotator)
					argType.Set<FRotator>(FRotator::ZeroRotator);
				else if (arg.Value == EEventArgTypes::UObjectPtr)
					argType.Set<UObject*>(nullptr);
				else if (arg.Value == EEventArgTypes::AActorPtr)
					argType.Set<AActor*>(nullptr);
				else if (arg.Value == EEventArgTypes::UEnum)
					argType.Set<uint8>(0);
				else if(arg.Value == EEventArgTypes::CustomStruct)
					argType.Set<FEventArgStruct*>(nullptr);

				ev->m_eventArgs.Add(CEventArg(arg.Key, argType));
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("%p"), this);

	
	Get("OnWeaponFired").GetDelegate().AddLambda([this](UGameEvent& ev)
	{
		UGameEventManager* sa = static_cast<UGameEventManager*>(ev.GetValue<UObject*>("GameEventManager"));
		UE_LOG(LogTemp, Warning, TEXT("%p"), sa);
	});

	Get("OnPlayerLanded").Broadcast(251.0f);
	Get("OnWeaponFired").Broadcast(static_cast<UObject*>(this));

	
}

void UGameEventManager::Clear()
{
	m_events.Empty();
}

UGameEventDynamic* UGameEventManager::GetDynamic(FName id, bool& success)
{
	if(!m_events.Contains(id))
	{
		success = false;
		return nullptr;
	}
	
	success = true;
	return static_cast<UGameEventDynamic*>(m_events[id]);
}

UGameEvent& UGameEventManager::Get(const FName& id)
{
	UGameEvent** ev = m_events.Find(id);
	check(ev != nullptr);
	return **ev;
}

void UGameEvent::CheckArgsCounter()
{
	m_argsCounter++;
	if (m_argsCounter >= m_eventArgs.Num())
	{
		BroadcastDelegate();
	}
}

void UGameEvent::BroadcastDelegate()
{
	m_delegate.Broadcast(*this);
}

void UGameEventDynamic::BroadcastDelegate()
{
	m_dynamicDelegate.Broadcast(this);
}
