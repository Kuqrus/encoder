#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <fstream>
#include <bitset>
#include <assert.h>

namespace ObjectModel{
	enum class Wrapper : int8_t {
		PRIMITIVE = 1,
		ARRAY,
		STRING,
		OBJECT
	};
	enum class Type : int8_t {
		I8 = 1,
		I16,
		I32,
		I64,

		U8,
		U16,
		U32,
		U64,

		FLOAT,
		DOUBLE,

		BOOL
	};

	class Root {
	public:
		int32_t GetSize();
		void SetName(std::string);
		std::string GetName();
		virtual void Pack(std::vector<int8_t>*, int16_t*);

	protected:
		std::string name;
		int16_t nameLength;
		int8_t wrapper;
		int32_t size;

		Root();
	};

	class Primitive : public Root {
	public:
		void Pack(std::vector<int8_t>*, int16_t*);

		template <typename T>
		static Primitive* Create(std::string name, Type type, T value);

	private:
		int8_t type = 0;
		std::vector<int8_t>* data = nullptr;

		Primitive();
	};

	class Array : public Root {
	public:
		template<typename T>
		static Array* CreateArray(std::string name, Type type, std::vector<T> value);

		template <typename T>
		static Array* CreateString(std::string name, Type type, T value);

		void Pack(std::vector<int8_t>*, int16_t*);


	private:
		int8_t type = 0;
		int32_t count = 0;
		std::vector<int8_t>* data = nullptr;

		Array();
	};

	class Object : public Root {
	public:
		Object(std::string);

		void AddEntitie(Root*);

		Root* FindByName(std::string);
		void Pack(std::vector<int8_t>*, int16_t*);

	private:
		std::vector<Root*> entities;
		int16_t count = 0;
	};
}

namespace EventSystem {

	class System;

	class Event {
	public:
		enum class DeviceType : int8_t {
			KEYBOARD = 1,
			MOUSE,
			TOUCHPAD,
			JOYSTICK
		};
		DeviceType dType;
		System* system = nullptr;

		Event(DeviceType);
		DeviceType GetdType();
		int32_t GetId();

		// If not work - make not enum class but enum and remove dtype:: (34:00 in vid)
		friend std::ostream& operator<<(std::ostream& stream, const DeviceType dType) {
			std::string res;
#define PRINT(a) res = #a;
			switch (dType)
			{
			case DeviceType::KEYBOARD: PRINT(KEYBOARD);
				break;
			case DeviceType::MOUSE: PRINT(MOUSE);
				break;
			case DeviceType::TOUCHPAD: PRINT(TOUCHPAD);
				break;
			case DeviceType::JOYSTICK:	PRINT(JOYSTICK);
				break;
			default:
				break;
			}
			return stream << res;
		}

		void Bind(System*, Event*);
		void Serialize(ObjectModel::Object*);
	private:
		int32_t id = 0;


	};

	class System {
	public:
		friend class Event;
		System(std::string);
		~System();
		void AddEvent(Event*);
		Event* GetEvent();
		void Serialize();
		//bool IsActive();

	private:
		std::string name;
		int32_t descriptor;
		int16_t index;
		bool acive;
		std::vector<Event*> events;
	};

	class KeyboardEvent : public Event {
	public:
		KeyboardEvent(int16_t, bool, bool);
		void Serialize(ObjectModel::Object*);

	private:
		int16_t keyCode;
		bool pressed;
		bool released;

	};
}

namespace Core {
	namespace Util {
		bool IsLittleEdian();
		void Save(const char* file, std::vector<int8_t> buffer);
		void RetriveAndSave(ObjectModel::Root* root);
	}


	template <typename T>
	void Encode(std::vector<int8_t>* buffer, int16_t* iterator, T value) {
		for (unsigned i = 0, j = 0; i < sizeof(T); i++) {
			(*buffer)[(*iterator)++] = (value >> ((sizeof(T) * 8) - 8) - (i * 8));
		}
	}

	template<>
	void Encode<float>(std::vector<int8_t>* buffer, int16_t* iterator, float value) {
		int32_t res = *reinterpret_cast<int32_t*>(&value);
		Encode<int32_t>(buffer, iterator, res);
	}

	template<>
	void Encode<double>(std::vector<int8_t>* buffer, int16_t* iterator, double value) {
		int64_t res = *reinterpret_cast<int64_t*>(&value);
		Encode<int64_t>(buffer, iterator, res);
	}

	template<>
	void Encode<std::string>(std::vector<int8_t>* buffer, int16_t* iterator, std::string value) {
		for (unsigned i = 0; i < value.size(); i++) {
			Encode<int8_t>(buffer, iterator, value[i]);
		}
	}

	template <typename T>
	void Encode(std::vector<int8_t>* buffer, int16_t* iterator, std::vector<T> value) {
		for (unsigned i = 0; i < value.size(); i++) {
			Encode<T>(buffer, iterator, value[i]);
		}
	}
}



namespace ObjectModel {
	/*
	*
	* DEFENITION
	*
	*/

	Root::Root() : name("UNKNOWN"), wrapper(0), nameLength(0), size(sizeof nameLength + sizeof wrapper + sizeof size) {}

	void Root::SetName(std::string name_) {
		this->name = name_;
		nameLength = static_cast<int8_t>(name.length());
		size += nameLength;
	}

	void Root::Pack(std::vector<int8_t>* v, int16_t* i) {
		//
		//
		//
	}

	int32_t Root::GetSize() { return size; }
	std::string Root::GetName() { return name; }

	Root* Object::FindByName(std::string name) {
		for (auto r : entities) {
			if (r->GetName() == name) {
				return r;
			}
		}
		std::cout << "No such object\n" << std::endl;
		return new Object("nullptr");
		//return nullptr;
	}


	Primitive::Primitive() {
		size += sizeof type;
	}

	void Primitive::Pack(std::vector<int8_t>* v, int16_t* i) {
		Core::Encode<std::string>(v, i, name);
		Core::Encode<int16_t>(v, i, nameLength);
		Core::Encode<int8_t>(v, i, wrapper);
		Core::Encode<int8_t>(v, i, type);
		Core::Encode<int8_t>(v, i, *data);
		Core::Encode<int32_t>(v, i, size);
	}

	template<typename T>
	static Primitive* Primitive::Create(std::string name, Type type, T value) {
		Primitive* p = new Primitive();
		p->SetName(name);
		p->wrapper = static_cast<int8_t>(Wrapper::PRIMITIVE);
		p->type = static_cast<int8_t>(type);
		p->data = new std::vector<int8_t>(sizeof value);
		p->size += p->data->size();
		int16_t iterator = 0;
		Core::template Encode<T>(p->data, &iterator, value);

		return p;
	}

	Array::Array() {
		size += sizeof type + sizeof count;

	}

	template<typename T>
	static Array* Array::CreateArray(std::string name, Type type, std::vector<T> value) {
		Array* arr = new Array();
		arr->SetName(name);
		arr->wrapper = static_cast<int8_t>(Wrapper::ARRAY);
		arr->type = static_cast<int8_t>(type);
		arr->count = value.size();
		arr->data = new std::vector<int8_t>(sizeof(T) * arr->count);
		arr->size += sizeof(T) * value.size();
		int16_t iterator = 0;
		Core::template Encode<T>(arr->data, &iterator, value);

		return arr;
	}

	template <typename T>
	static Array* Array::CreateString(std::string name, Type type, T value) {
		Array* str = new Array();
		str->SetName(name);
		str->wrapper = static_cast<int8_t>(Wrapper::STRING);
		str->type = static_cast<int8_t>(type);
		str->count = value.size();
		str->data = new std::vector<int8_t>(value.size());
		str->size += value.size();
		int16_t iterator = 0;
		Core::template Encode<T>(str->data, &iterator, value);

		return str;
	}

	void Array::Pack(std::vector<int8_t>* v, int16_t* i) {
		Core::Encode<std::string>(v, i, name);
		Core::Encode<int16_t>(v, i, nameLength);
		Core::Encode<int8_t>(v, i, wrapper);
		Core::Encode<int8_t>(v, i, type);
		Core::Encode<int32_t>(v, i, count);
		Core::Encode<int8_t>(v, i, *data);
		Core::Encode<int32_t>(v, i, size);
	}

	Object::Object(std::string name) {
		SetName(name);
		wrapper = static_cast<int8_t>(Wrapper::OBJECT);
		size += sizeof count;
	}

	void Object::AddEntitie(Root* r) {
		this->entities.push_back(r);
		count += 1;
		size += r->GetSize();
	}

	void Object::Pack(std::vector<int8_t>* v, int16_t* i) {
		Core::Encode<std::string>(v, i, name);
		Core::Encode<int16_t>(v, i, nameLength);
		Core::Encode<int8_t>(v, i, wrapper);
		Core::Encode<int16_t>(v, i, count);

		for (auto r : entities) {
			r->Pack(v, i);
		}

		Core::Encode<int32_t>(v, i, size);
	}
}

namespace EventSystem {
	/*
	*
	* DEFENITION
	*
	*/

	System::System(std::string name_) : name(name_), descriptor(123), index(1), acive(true) {};
	System::~System() {
	}

	void System::AddEvent(Event* event) {
		event->Bind(this, event);
	}

	Event* System::GetEvent() { return events.front(); }

	//bool System::IsActive() { if (!system) return false; return acive; }

	void System::Serialize() {
		ObjectModel::Object system("SysInfo");
		ObjectModel::Array* name = ObjectModel::Array::CreateString("sysName", ObjectModel::Type::I8, this->name);
		ObjectModel::Primitive* desc = ObjectModel::Primitive::Create("desc", ObjectModel::Type::I32, this->descriptor);
		ObjectModel::Primitive* index = ObjectModel::Primitive::Create("index", ObjectModel::Type::I16, this->index);
		ObjectModel::Primitive* active = ObjectModel::Primitive::Create("active", ObjectModel::Type::BOOL, this->index);
		system.AddEntitie(name);
		system.AddEntitie(desc);
		system.AddEntitie(index);
		system.AddEntitie(active);

		for (Event* e : events) {
			KeyboardEvent* kb = static_cast<KeyboardEvent*>(e);
			ObjectModel::Object* eventObject = new ObjectModel::Object("Event: " + std::to_string(e->GetId()));
			kb->Serialize(eventObject);
			system.AddEntitie(eventObject);
		}

		Core::Util::RetriveAndSave(&system);
	}

	Event::Event(DeviceType dType_) {
		std::random_device rd;
		std::uniform_int_distribution<> destr(1, 100);
		this->id = destr(rd);
		this->dType = dType_;
	}

	void Event::Bind(System* sys, Event* event) {
		this->system = sys;
		this->system->events.push_back(event);
	}

	void Event::Serialize(ObjectModel::Object* o) {
		ObjectModel::Primitive* id = ObjectModel::Primitive::Create("ID", ObjectModel::Type::I32, this->GetId());
		ObjectModel::Primitive* dType = ObjectModel::Primitive::Create("dType", ObjectModel::Type::I8, static_cast<int8_t>(this->GetdType()));

		o->AddEntitie(id);
		o->AddEntitie(dType);
	}

	Event::DeviceType Event::GetdType() { return this->dType; }

	int32_t Event::GetId() { return id; }


	KeyboardEvent::KeyboardEvent(int16_t keyCode_, bool pressed_, bool released_) : Event(Event::DeviceType::KEYBOARD), keyCode(keyCode_), pressed(pressed_), released(released_) {}

	void KeyboardEvent::Serialize(ObjectModel::Object* o) {
		Event::Serialize(o);

		ObjectModel::Primitive* keyCode = ObjectModel::Primitive::Create("keyCode", ObjectModel::Type::I16, this->keyCode);
		ObjectModel::Primitive* pressed = ObjectModel::Primitive::Create("pressed", ObjectModel::Type::BOOL, this->pressed);
		ObjectModel::Primitive* released = ObjectModel::Primitive::Create("released", ObjectModel::Type::BOOL, this->released);

		o->AddEntitie(keyCode);
		o->AddEntitie(pressed);
		o->AddEntitie(released);
	}
}

namespace Core {
	/*
	*
	* DEFENITION
	*
	*/

	namespace Util {
		bool IsLittleEdian() {
			int8_t a = 5;
			std::string res = std::bitset<8>(a).to_string();
			if (res.back() == '1') return true;
			return false;
		}

		void Save(const char* file, std::vector<int8_t> buffer) {
			std::ofstream out;
			out.open(file);

			for (unsigned i = 0; i < buffer.size(); i++) {
				out << buffer[i];
			}
			out.close();
		}

		void RetriveAndSave(ObjectModel::Root* root) {
			int16_t iterator = 0;
			std::vector<int8_t> buffer(root->GetSize());
			std::string name = root->GetName().substr(0, root->GetName().length()).append(".abc");
			root->Pack(&buffer, &iterator);
			Save(name.c_str(), buffer);
		}
	}
}



using namespace EventSystem;
using namespace ObjectModel;



int main()
{
	assert(Core::Util::IsLittleEdian());


#if 0
	
	int32_t foo = 5;
	Primitive* p = Primitive::Create("int32", Type::I32, foo);
	Core::Util::RetriveAndSave(p);

	std::vector<int64_t> data{ 1,2,3,4 };
	Array* arr = Array::CreateArray("Array", Type::I64, data);
	Core::Util::RetriveAndSave(arr);

	std::string name = "name";
	Array* str = Array::CreateString("String", Type::I8, name);
	Core::Util::RetriveAndSave(str);

	Object Test("Test");
	Test.AddEntitie(p);
	Test.AddEntitie(arr);
	Test.AddEntitie(str);

	Object Test2("Test2");
	Test2.AddEntitie(p);
	Core::Util::RetriveAndSave(&Test2);

	Test.AddEntitie(&Test2);
	Core::Util::RetriveAndSave(&Test);

#endif

	
	System Foo("Foo");
	Event* e = new KeyboardEvent('W', true, false);
	Foo.AddEvent(e);

	KeyboardEvent* kb = static_cast<KeyboardEvent*>(Foo.GetEvent());
	Foo.Serialize();
	
	

	std::cout << "Hello World!\n";
}