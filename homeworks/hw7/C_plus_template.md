[toc]

# 0. 序言

​	C#中的模板类型约束很好用, C++中要想使用就很复杂



# 1. 使用未定义的类型实例化模板类

```c++
template <typename TD>
class D{
};

template <typename TC>
class C{
  TC*c1;
};

template <typename TB>
class B{
  TB b1;  // error. Because A do not definition. Its size is unkonw.
  TB* b2;  // ok. Pointer size is fixed(4 bytes generatly).
  C<TB> b3;  // ok. TB will be the pointer type in C template.
  D<TB> b4;  // ok. Class D do not use template type TB.
};

class A;  // Forward declare;

class A : public B<A>{
  ...  // some code.
};

```



# 2. 模板类中使用未定义类型的指针和其方法.

```c++
class C {
};

template <typename TB>
class B {
public:
	// These codes make we can use derive class method in base class
	TB* This() { return static_cast<TB*>(this); }
  // 使用的是未定义类型的 指针的 方法.
	void methodB() { This()->methodA(); }  // ok. It is a good traits, because we can use any method through pointer. In spite of it is not a specific type.

	// warning. But this will cause some problem. May template type TB has not relative method.
	void methodB2() { This()->methodA2(); }  // error(but compile passed).
};

class A;

class A : public B<A> {  // pay attention A is not definition now.
public:
	void methodA() {};
};

int main(void) {
	A a;
	a.methodB();  // ok
	a.methodB2();  // error. A has not methodA2().
  
	C* c;
	c->methodC();  // error(compile failed)
}
```

​	联想到只有实例化模板时, 才能产生具体类.

​	猜测实现它的方式是**将模板方法产生具体方法的过程, 从模板类实例化中抽离出来(或是延迟)**. 也就是说, 当你实例化某个模板类, 但没有调用它的某个模板方法, 该模板方法不会具体化.

```c++
class A;

template <typename TB>
class B {
public:
    B(TB* a) {
        this->a = a;
    }

    void methodB() {
        a->methodA();  // compile passed
    }

    TB* a;
};

A* a = nullptr;  // danger!
B<A> b(a);

class A {
public:
    //void methodA() {};
};

int main(void) {
		b.methodB();
    return 0;
}
```

​	new操作符实例也佐证了这个想法. 至少表象上这个解释是通的.



# 3. new操作符

```c++
// #include <malloc.h> ignorn this
#include <iostream>

class A;

template <typename TB>
class B {
public:
    //TB* b = malloc(sizeof(TB));  ignorn this too
    TB* a = nullptr;
    // You can use TB as an local varible or use by new keyword.
    void methedB() {
        TB a1;
        a = new TB();
        // new (a) TB(); 指定内存空间中产生新对象
        a1.methodA();  // ok
        a->methodA();  // ok
    }

    ~B() {
        if (a != nullptr) delete a;
    }
};

B<A> b;

class A {
public:
    void methodA() {
        std::cout << "hello world" << std::endl;
    };
};

int main(void) {
    b.methedB();
    return 0;
}
```



# 4. Template(Genericity type) constraint

​	为了保证模板参数代表的类一定含有指定的方法. 可以使用模板约束.

## 4.1 使用constexpr定义编译期间的变量或是函数

```c++
bool constexpr isVaild(){
  return false;
}
```

## 4.2 static_assert编译期间检测

```c++
static_assert(isVaild());
```

## 4.3 一些内置constexpr函数(来自type_traits文件)

```c++
std::is_base_of_v<BaseClass, ClassA>
std::is_default_constructible_v<ClassA> 等
```

## 4.4 例子

```c++
#include <type_traits>

// Use this class to limit
class C {
    virtual void methodA() = 0;
};

class A : public C {
    virtual void methodA() {};
};

template <typename TB>
class B {
  	// 在实例化模板的时候, 检查模板参数是否是指定类型的派生.
    static constexpr bool isVaild() noexcept {
        return std::is_base_of_v<C, TB>;
    };

    // guarantee class A has all method in virtual class C.
    static_assert(isVaild());

    void methodB() {
        TB a;
        a.methodA();
    }
};

int main(void) {
    B<A> b;
    return 0;
}
```
