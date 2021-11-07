# UE-Event-Manager-EditorExposed
Simple C++ Event Manager that is exposed to UE4 editor, meaning that one can define events using DataTables within Editor instead of hard-coding them in C++.

## What?

This is a simple event manager to use in your UE4 projects. Unlike most other event manager/system implementations, it focuses on the ability to create & edit events on Editor without re-compiling the source code for adding a new event or a new parameter. It uses TVariants for type checking and supports most of the basic types including numeric types, UObject, AActor and custom user structs.

## Usage Example

User creates a data table in Editor, defining events along with their parameters.

![image](https://user-images.githubusercontent.com/3519379/140647283-2dfa5305-25c2-436f-b547-14c9b6712019.png)

Then fire events in C++:

```cpp

// Get the event manager from somewhere, you can create a singleton, or create a member var in GameMode or GameInstance
UGameEventManager* manager = GetEventManager();

// Broadcast the event
manager->Get("OnPickupItem").Broadcast(FName("9mmAmmo"), 17);

```

The events can only be fired in C++ (might change later), but can be listened in both C++ as well as Blueprints.

## Why?

This is merely a hobby-project I've written in a day. One of the fastest ways to deliver events is just using multicast-delegates to hard-define the event types/arguments in C++ code, there is no doubt. However, I wondered if it would be possible to write a more flexible system one can edit through the editor without touching the C++ and came up with this.

## Installation

### Sources

It's just a header & cpp file. Just include them in your project directory, or simply create a new class of type UObject called GameEventManager. Then copy and paste the contents.

### Object Creation

Now you would have a UObject class, UGameEventManager. You first need to create a blueprint class of this class. Right click on your content browser, select Blueprint Class and select GameEventManager as the base class.

Then you need to create a DataTable. Right click on content browser > Miscellaneous > DataTable and select the row type as **EventDefinition**. Now you'd have two assets:

![image](https://user-images.githubusercontent.com/3519379/140647952-ef6bad8c-438f-40d5-ab58-a6212cb46f65.png)

Then go to the GameEventManager blueprint class you've just created and assign the data table you've created.

![image](https://user-images.githubusercontent.com/3519379/140647971-92bf0e81-bb6b-47c1-91bc-93bc82783756.png)

### Setup

Last step to setup is that you need to instantiate the UObject somewhere and then call **Setup()** on it. This is up to you and your project, here is an example for instantiating it in your custom GameMode class & setting up a global static access.

Header
```cpp

// AMyGameMode.h

// Forward decl.
class UGameEventManager;

public:

  static UGameEventManager* EventManager;
  
protected:

  virtual void StartPlay() override;
  
private:

  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
  TSubclassOf<UGameEventManager> m_eventManagerClass;
  
  UPROPERTY()
  UGameEventManager* m_eventManager = nullptr;

```

Cpp
```cpp

// AMyGameMode.cpp

UGameEventManager* AMyGameMode::EventManager = nullptr;

void AMyGameMode::StartPlay()
{
  // Instantiate & call setup.
  m_eventManager = NewObject<UGameEventManager>(this, m_eventManagerClass.Get());
  m_eventManager->Setup();
  EventManager = m_eventManager;
}

```
Of course this is just an example asssuming you are using a custom game mode class. You can handle these steps in any global environment you've setup. The main idea is that this is a UObject class, you need to create a blueprint class from, assign the event data table and instantiate that class, then call **Setup()** on the instance.

## Detailed Usage

Now you can setup your events in the Event Data Table, prepare the argument lists there. Afterwards you can use your events in your gameplay logic.

Fire an event:

```cpp

// Fire an event with no params defined in event table.
EventManager->Get("OnPlayerJumped").Broadcast();

// Parameter order needs to be the same as it is defined in the Event Table, otherwise system will complain and won't broadcast the event.

// Fire an event with basic params.
EventManager->Get("OnPickupItem").Broadcast(FName("Ammo9mm"), 17);

// Fire an event with enum and actor pointer.
AMyWeaponActor* actor = GetWeaponActor();
const EWeaponType weaponType = actor->GetType();
EventManager->Get("OnWeaponFired").Broadcast(static_cast<uint8>(weaponType), static_cast<AActor*>(actor));

// Fire custom struct (more on the FEventArgStruct later)
FBulletHitParameters params;
params.m_kineticEnergy = 824;
EventManager->Get("OnBulletHit").Broadcast(static_cast<FEventArgStruct*>(&params));

```

Listen to an event:

```cpp

// The GetDelegate() method returns your typical multicast delegate with function signature: UGameEvent& event
// So you can bind ufunctions, uobjects, sps, lambdas, basically the same operations as UE's multicast delegates.
EventManager->Get("OnPlayerJumped").GetDelegate().AddLambda([](UGameEvent& ev){});
EventManager->Get("OnPlayerJumped").GetDelegate().AddUObject(this, &........);

```

Receive an event argument:


```cpp

// Use GetValue<T> method to get a value of the desired type.
// System will log an error and return the default value of T if you try to get a type or a variable that does not exists.
EventManager->Get("OnClimbSurface").GetDelegate().AddLambda([](UGameEvent& ev)
{
  FRotator orientation = ev.GetValue<FRotator>("Orientation");
  FVector base = ev.GetValue<FVector>("Base");
  FString tag = ev.GetValue<FString>("Tag");
});

// Use GetValue for a pointer type, then down-cast to target ptr.
EventManager->Get("OnWeaponFired").GetDelegate().AddLambda([](UGameEvent& ev)
{
  AMyWeaponActor* actor = static_cast<AMyWeaponActor*>(ev.GetValue<AActor*>("WeaponActor"));
  EWeaponType weaponType = static_cast<EWeaponType>(ev.GetValue<uint8>("WeaponType"));
});

```

## Dynamic Events

For the events that you'd like to listen to in Blueprints, you need to mark them as dynamic by checking "Is Dynamic?" property in the Event Table. If you also would like to listen to the events market with dynamic in C++, you need to make slight modifications:

```cpp

// Instead of Get(), use GetDynamic, instead of a reference it returns a pointer.
// Instead of GetDelegate(), use GetDynDelegate(), instead of a multicast delegate it returns a dynamic multicast delegate. 
// Then you can fire or listen to events same way as non-dynamic events
EventManager->GetDynamic("MyDynamicEvent")->GetDynDelegate().AddDynamic();

```

To listen to events in blueprints, get a reference to the Event Manager, get a dynamic event by name, bind to it's delegate, then use GetInt(), GetFloat(), GetFVector() etc.
functions to get a desired value by variable name.


![image](https://user-images.githubusercontent.com/3519379/140648541-ca4d7a46-7742-404d-ac6a-8c0ac3b389d8.png)

**Getting custom structs from the events are not supported for blueprints.**

## Limitations & Important Info

### Need for Casts

We can broadcast & receive events with any type of parameters packed in as arguments, floats, ints, UObjects, AActors etc., all this is because these types are defined in the variant initialization:

```cpp
/**
* Allowed base types.
*/
typedef TVariant<int, float, double, FString, FName, bool, FVector, FVector2D, FRotator, UObject*, AActor*, uint8, FEventArgStruct*> EventArgType;
```

Since the variant doesn't know about your custom Actors or UObjects, you need to send them as base class pointers. Then you need to receive them as base class pointers as well,
only then you can down-cast to your actual type.

### FEventArgStruct

The casting above works for all types of UObjects, Actors, enums etc. but what about custom user defined structs in C++ code? For that, there is an empty struct FEventArgStruct, which you can use to derive your custom structs from. 

```cpp

USTRUCT(BlueprintType, Blueprintable)
struct FBulletHitParams : public FEventArgStruct
{
  GENERATED_BODY()
  float m_kineticEnergy = 0.0f;
}

```

Thus you will be able to broadcast your custom struct value as a casted pointer to the FEventArgStruct type, then receive it the same way. Afterwards, you can down-cast to your own struct type.

```cpp

// Fire custom struct
FBulletHitParameters params;
params.m_kineticEnergy = 824;
EventManager->Get("OnBulletHit").Broadcast(static_cast<FEventArgStruct*>(&params));

// Get custom struct
FBulletHitParameters params = *static_cast<FBulletHitParameters*>(EventManager->Get("OnBulletHit").GetValue<FEventArgStruct*>("BulletParams"));

```

Unfortunately, this casting is not supported on blueprints.
