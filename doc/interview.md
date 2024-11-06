# Interview

## Topics

- Классические алгоритмы и структуры данных (DSA), оценка сложности
    - Поиск (в списке, массиве, упорядоченном массиве, дереве, хеш-таблице)
    - Вставка/удаление (в массив, список, бинарное дерево, хеш-таблицу)
    - Бинарное дерево поиска (BST), проблема балансировки BST,
      сбалансированные по построению деревья (B-Trees), 2-3-4-дерево и
      его бинарное представление -- красно-чёрное дерево
    - Хеш-таблица, хеш-функция (отображение записи произвольной длины в запись
      фиксированной длины), коллизия, свойства хеш-функции, идеальное хеширование
    - Сортировка: быстрая (quicksort, Hoar), пирамидальная (heap sort),
      слиянием (merge sort), сортировка с помощью BST.  Какая удобнее для
      различных случаев и структур данных
        - Стабильность сортировки
        - Оценка сложности: лучший, худший, средний случай
    - Перестановки, размещения, сочетания: перебор и оценка кол-ва вариантов
- Симметричное и асимметричное шифрование цифровая подпись
- Динамическое распределение памяти (первый подходящий, последний
  подходящий, система близнецов и т. д.)
- Процесс загрузки операционной системы (загрузчик, ядро, init)
- Работа простой вычислительной машины (MIX, 8080 и т. д.)
- Работа простого загрузчика (например, LittleKernel)
- ELF-format, структура, типичные секции и сегменты
- Процесс загрузки статического исполняемого файла
- Процесс загрузки исполняемого файла с разделяемыми библиотеками (.so),
  назначение /lib/ld.so
- Средства разработки
    - Компилятор (этапы преобразования исходного файла в ELF-файл, на
      примере gcc: `.cc -> .i -> .S -> .o -> .so`)
    - Редактор связей (ld, linker), области видимости символов
    - Отладка: gdb, valgrind, strace.  Принцип работы (ptrace, instrumenting)
    - Оценка производительности: perf/gprof (performance counters, instrumenting)
    - Система контроля версий исходников: git (interactive rabase, branch
      cherry-picking, resolving conflicts)
- Интерпретатор, выполнение байт-кода (portable code)
- Эмуляция машины целиком или отдельно userspace с помощью qemu (Linux Binary
  Formats (binfmt), qemu-system, qemu-user)
- Изоляция адресного пространства процессов
    - Гранулярность управления физической и логической памяти
    - Страничная организация, кеширование адресов страниц (paging, page frame, pfn, TLB)
        - [How does x86 paging work?](https://stackoverflow.com/questions/18431261/how-does-x86-paging-work)
        - [What happens after a L2 TLB miss?](https://stackoverflow.com/questions/32256250/what-happens-after-a-l2-tlb-miss)
    - Отличие процесса от потока
    - Кеширование памяти, уровни, быстродействие, ассоциативность,
      true-одновременный доступ к памяти, барьеры
    - Non-uniform memory access (NUMA)
- Linux
    - Конфигурирование и сборка из исходников
    - Написание модулей, подпись модулей ядра
    - namespaces (mount, pid, user, ...)
    - Control Groups (cgroups)
    - Linux Security Modules (LSM), SELinux
    - Назначение и функции init (pid == 1) (SystemV init, systemd)
    - Initrd и initramfs
    - Integrity Measurement Architecture (IMA) and Extended Verification Module (EVM)
    - Файловые системы
        - Filesystem superblock
        - inode
        - File/directory
        - Ownership
        - Access/Create/Modify time
        - Access Permissions
        - Extended Attributes
        - Advanced Control Lists (ACL)
        - link/unlink
        - symlink vs hardlink
        - inline data
        - debugfs (?)
    - Виртуальные файловые системы (procfs, sysfs, configfs, overlayfs, ...)
    - DBus (system/session Bus, message types, service activation,
      access permissions, programming language bindings)
    - Взаимодействие процесса с ядром
        - Копирование данных между адресными пространствами
        - Системный вызов/syscall/system call
        - Специальные файлы в /dev.  В чём отличие блочных от символьных?
        - /proc
        - /sysfs
        - ioctl
    - Взаимодействие процессов (shmem, pipes, System V IPC, unix domain
      sockets, файлы, ...)
    - Ожидание наступления события, например появление данных в
      сокете (sleep vs poll)
    - likely/unlikely
    - kmalloc vs vmalloc
    - memory barrier
    - device mapper
    - caps, capabilities
    - how file is found by name
    - how fs quota works
- Средства синхронизации
    - Гонка процессов и потоков (race condition) за ресурсы
    - Типичные сценарии использования блокировок (mutex: lock/unlock, semaphore:
      wait/signal, RCU: read-copy-update, spin-lock, conditional variables)
    - Atomic operations, преимущества и ограничения
    - Способы избежать необходимости в блокировках, per-thread data, per-cpu data
- Пакеты библиотек Qt, Glib (event loop, signal/slot)
- Пользовательский интерфейс: Qt/QML (декларативное программирование,
  переход к императивному с помощью встроенного java-script)

Also

- CPU Cache

- CPU Stack

    - Heap memory allocation vs Stack memory allocation:

        - Stack Memory

            - This memory space stores local variables
            - When allotted (by OS) stack memory gets filled, Stack Overflow error occurs
            - Data saved on the stack can only be accessed by the owner thread, making it safer
            - Stack frame access is easier (only change Stack Pointer register)
            - Potential threat: Shortage of memory
            - The excellent locality of reference
            - Allocation time is much faster than Heap memory allocation

        - Heap Memory

            - This memory space stores dynamic variables
            - When allocated (by OS) heap memory gets filled, Heap Overflow error occurs
            - Heap memory is not safest as data stored in Heap-memory is visible to all threads
            - Heap frame access is difficult (request to the OS)
            - Potential threat: Memory Fragmentation
            - The adequate locality of reference (?)
            - Allocation time is much slower than the stack memory allocation

    - Stack and multi-threading
    - Stack growth direction, why?

        - [What is the direction of stack growth in most modern systems?](https://stackoverflow.com/questions/664744/what-is-the-direction-of-stack-growth-in-most-modern-systems)
        - Having the stack grow downwards makes all addresses within the stack have a
          positive offset relative to the stack pointer.

    - How does a process stack grow automatically?

        - [How does stack allocation work in Linux?](https://unix.stackexchange.com/questions/145557/how-does-stack-allocation-work-in-linux)
        - [What is "automatic stack expansion"?](https://unix.stackexchange.com/questions/63742/what-is-automatic-stack-expansion)

- мьютекс, семафоры. Pthread. Shared memory, mqueue
- Conditional variables, `std::condition_variable`
- producer/consumer
- text, bss (Block Started by Symbol on IBM 7090 assembly), data, rodata
- How `.text`, `.data`, `.rodata`, `.bss` are related to runtime memory mappings?
- i2c vs spi vs can bus
- how unix signals are delivered
- how qt signals are delivered ?
- Why `int main()`, `int main(int argc, char *argv[])` and
  `int main(int argc, char *argv[], char *envp[])` are interchangeable?
    - [When main is defined without parameters, will argc and argv still be present on the stack?](https://stackoverflow.com/questions/3144316/when-main-is-defined-without-parameters-will-argc-and-argv-still-be-present-on)
    - [What is the difference between the definitions `int main (void)` and `int main (int argc, char *argv[])` and when to use which?](https://stackoverflow.com/questions/66449569/what-is-the-difference-between-the-definitions-int-main-void-and-int-main)
- Heap allocation vs mmap-ing pages
- [Why do system calls exist?](https://stackoverflow.com/questions/50626460/why-do-system-calls-exist)
    - A good comment: Read Operating Systems: Three Easy Pieces since an
      entire book is needed to answer your question – Basile Starynkevitch Commented May 31, 2018 at 14:30
- `copy_to_user()`, `copy_from_user()`

### Optional

- Взаимодействие узлов сети (client/server, BSD-sockets: tcp, udp), MPI, 
  Ethernet/ib/opath/..., прямой доступ к памяти RDMA

## Questions

- rpm: `Requires` vs `BuildRequires`, `Conflicts` vs `Obsoletes`, what is `%check` for?
- C function prototype accepting an argument of any type
    ```c
    void foo(const void *key, size_t key_size);
    ```
- How this compiles, links and runs?

    ```c
    int main;
    ```

    ```
    static int main()
    {
        return 0;
    }
    ```
- Linker and symbol visibility: `static` function vs anonymous `namespace`

    ```c++
    namespace {
    static int foo() {...}
    }

    namespace {
    int bar() {...}
    }

    static int baz() {...}
    ```
- Where semicolon is optional?
    ```c++
    struct A {
        A() {};
        explicit A(int a) {};
    private:
        get_value() const {return m_value};
        set_value(int a)
        {
            m_value = a;
        };
    };
    ```

    See also:<br>
    - [Optional semicolon in C++](https://stackoverflow.com/questions/11376025/optional-semicolon-in-c)

- What's difference between `std::array` vs `std::vector` (Also,
  heap memory allocation vs Stack memory allocation)

- ABI vs API compatibility
    - API
        - Requires changing the code
    - ABI
        - How parameters are passed to functions (registers/stack)
        - Who cleans parameters from the stack (caller/callee)
        - Where the return value is placed for return
        - How exceptions propagate (??)

- How to ensure ABI compatibility

    - Pimpl (Private Implementation), opaque pointer

        ```c++
        struct A {
        private:
            int m_value;
        public:
            int value() const;
        };

        struct A {
        private:
            int m_new_value;
            int m_value;
        public:
            int value() const;
            int new_value() const;
        };

        struct A {
        private:
            int m_value;
            int m_new_value;
        public:
            int value() const;
            int new_value() const;
        };
        ```

    - [Pimpl idiom vs Pure virtual class interface](https://stackoverflow.com/questions/825018/pimpl-idiom-vs-pure-virtual-class-interface)

- Programming taste
    - Poor

        ```c
        void remove_entry_v1(node **head, node *entry)
        {
            node *prev = NULL;
            node *walk = *head;

            while (walk != entry) {
                prev = walk;
                walk = walk->next;
            }

            if (!prev) {
                *head = entry->next;
            } else {
                prev->next = entry->next;
            }

            free(entry);
        }
        ```

    - Good

        ```c
        void remove_entry_v2(node **head, node *entry)
        {
            while ((*head) != entry) {
                head = &(*head)->next;
            }

            *head = entry->next;

            free(entry);
        }
        ```

    - Why `if`-s are bad?
        - Branch prediction vs data prefetching
- Сложить два числа в двоичной записи: 1011010101 + 101010001101 = ?
- What is negative value of `a` in a 8-bit machine?

    ```c++
    int8_t a = 0b01011101;
    int8_t b = -a; // 0b???????? in two's complement?
                   // two's complement: 0b10100011

    // v1: substract n-bit number from a 2^n
    int8_t c1 = 0b100000000 - a;

    // v2: a simpler/faster equivalent of the subtraction:
    //     invert all bits and add 1
    int8_t c2 = ~a + 1;

    // v3: let the compiler do the work
    int8_t c3 = -a;
    ```
- Что значит «быстрая» сортировка (quick sort, Hoare's sort), что делает её быстрой?
  - Как бы вы реализовали параллельную быструю сортировку (в смысле использования
    нескольких ядер ЦП для сокращения длит. выполнения)?
  - 503 087 512 061 908 170 897 275 653 426 154 509 612 677 765 703
- Что значит vdso?
- Associativity:

    ```c++
    int a = 1, b = 2, c = 3;
    a = b = c;
    (a = b) = c;
    a = (b = c);
    ```

- Compiling steps: `.cc/cpp` -> `.ii` -> `.s` -> `.o` -> `a.out`

    ```shell
    $ gcc helloworld.c
    ```

- Why TLB cache was introduced in the modern CPUs, what task does it solve?
    ```
    Process A:   0x1000  <- 'a'  (store from a register)
    Process B:   0x1000  <- 'b'  (store from a register)
    Process C:   0x1000  ->  c   (load into a register)
    ```
- What for `addr` hint is used in `mmap`?
- What problem solves the buddy allocator?

    - red-rocket-computing / Buddy Allocator<br>
      https://github.com/red-rocket-computing/buddy-alloc/tree/master

- What's going on?

    ```shell
    (echo hello; echo world) > output.txt
    ```

- Relocatable vs. Position-Independent Code (See
  ["Relocatable vs. Position-Independent Code"](http://davidad.github.io/blog/2014/02/19/relocatable-vs-position-independent-code-or/)
  blog post [5] for some details)
- What is the Difference Between Object File and Executable File, and Shared Object, Static Archive?
- Семантическое версионирование библиотек (`so-name`)
- JTAG-debugger preloader, lk, uBoot, Linux kernel
- Smart pointers C/C++, how implemented
- Systemd unit limits, cgroup, isolation, containers
- GDB
    - Breakpoints: hardware/software (an illegal instruction)
    - Single stepping
    - Watchpoints
    - Checkpoints
    - Backtrace (stack frame)
- Valgrind how works
- Rotations in a binary tree
    - Left rotation
    ```
                3
               / \
              a   5
                 / \
                b   7
                   / \
                  c   d

               left(3)
                 ->

                 5
               /   \
              3     7
             / \   / \
            a   b c   d

    Left rotation of the grandparent (3) of
       the node that caused imbalance (7)
    ```

    ```c
    node *rotate_left(node *n)
    {
        node *new_root = n->right;
        n->right = new_root->left;
        new_root->left = n;
        return new_root;
    }
    ```
    Note: $`b`$ subtree may consist of nodes like $`b_i = \{3+i/(i+1)\}`$.
    - Right rotation
    ```
                7
               / \
              5   d
             / \
            3   c
           / \
          a   b

             right(7)
               ->

                5
              /   \
             3     7
            / \   / \
           a   b c   d

    Left rotation of the grandparent (7) of
      the node that caused imbalance (3)
    ```

    ```c
    node *rotate_right(node *n)
    {
        node *new_root = n->left;
        n->left = new_root->right;
        new_root->right = n;
        return new_root;
    }
    ```
    - Left-right rotation
    ```
                7
               / \
              3   d
             / \
            a   5
               / \
              b   c

            left(5)
              ->

                7
              /   \
             5     d
            / \
           3   c
          / \
         a   b

            right(7)
              ->

                5
              /   \
             3     7
            / \   / \
           a   b c   d

    Left-right rotation of the grandparent (7) of
         the node that caused imbalance (5)
    ```

    ```c
    node *rotate_left_right(node *n)
    {
        n->left = rotate_left(n->left);
        return rotate_right(n);
    }
    ```
    - Right-left rotation
    ```
                3
               / \
              a   7
                 / \
                5   d
               / \
              b   c

             right(7)
               ->

                3
              /   \
             a     5
                  / \
                 b   7
                    / \
                   c   d

              left(3)
                ->

                5
              /   \
             3     7
            / \   / \
           a   b c   d

    Right-left rotation of the grandparent (3) of
         the node that caused imbalance (5)
    ```

    ```c
    node *rotate_right_left(node *n)
    {
        n->right = rotate_right(n->right);
        return rotate_left(n);
    }
    ```
- Bit population count

    ```c++
    int popcnt(unsigned v)
    {
        v = ((v & 0xaaaaaaaa) >> 1) + (v & 0x55555555);  // 10101010 + 01010101
        v = ((v & 0xcccccccc) >> 2) + (v & 0x33333333);  // 11001100 + 00110011
        v = ((v & 0xf0f0f0f0) >> 4) + (v & 0x0f0f0f0f);  // 11110000 + 00001111
        v = ((v & 0xff00ff00) >> 8) + (v & 0x00ff00ff);  // ...
        v = ((v & 0xffff0000) >> 16) + (v & 0x0000ffff);

        return v;
    }
    ```
- Bit reverse?
- C++ placement new operator
- Move semantics (std::move).
    - Also, on the destructor, from the Herb Sutter's article [1]:
        > "Guideline #4: A base class destructor should be either
        > public and virtual, or protected and nonvirtual."

      But if the class is final (as in this case), inheritance is impossible, so a
      non-virtual destructor doesn't cause an undefined behaviour.  See also
      the discussion on StackOverflow [2] and a
      ["virtual functions"](http://www.dietmar-kuehl.de/mirror/c++-faq/virtual-functions.html)
      section in Marshall Cline's C++ FAQ LITE [4]

    - C++ std::vector emplace vs insert [duplicate]<br>
      https://stackoverflow.com/questions/14788261/c-stdvector-emplace-vs-insert

    - Avoiding implicit conversion in constructor. The 'explicit' keyword doesn't help here<br>
      https://stackoverflow.com/questions/57293752/avoiding-implicit-conversion-in-constructor-the-explicit-keyword-doesnt-help

    - What is a converting constructor in C++ ? What is it for?
      https://stackoverflow.com/questions/15077466/what-is-a-converting-constructor-in-c-what-is-it-for

    ```c++
    A::A(A &&other) : data{other.data}
    {
        other.data = nullptr;
    }
    ```

    or

    ```
    A::A(A &&other)
    {
        *this = other;
    }

    A &A::operator=(A &&other)
    {
        if (data) {
            delete data;
        }
        data = other.data;
        other.data = nullptr;
        return *this;
    }
    ```

    ```c++
    class A final {
    public:
        A();
        explicit A(int data);
        explicit A(const A &other);
        ~A();
    private:
        int *m_data;    // some complex structure
    };

    static std::ostream &operator<<(std::ostream &o, const A &a)
    {
        return (o << " {" << (a.data ? *a.data : 0) << "}");
    }

    A::A(int data) : m_data{new int}
    {
        *m_data = data;
    }

    A::A(const A &other) : A(*other.data)
    {
    }

    A::~A()
    {
        delete m_data;
    }

    int main()
    {
        std::map<std::string, A> amap;

        A a{123};
        amap.insert(std::make_pair(std::string{"key1"}, a));

        // Copy-constructor:
        //
        // create a temporary A22
        //    A::A(int) this 0x1ffefffbd8 data{0x61a9c80:22}
        // copy this temporary to a pair
        //    A::A(int) this 0x1ffefffc60 data{0x61aa110:22}
        //    A::A(const A&) this 0x1ffefffc60 data{0x61aa110:22} other 0x1ffefffbd8 data{0x61a9c80:22}
        // copy this temporary to a map
        //    A::A(int) this 0x61aa1a0 data{0x61aa1f0:22}
        //    A::A(const A&) this 0x61aa1a0 data{0x61aa1f0:22} other 0x1ffefffc60 data{0x61aa110:22}
        amap.insert(std::make_pair(std::string{"key1"}, A{22}));

        // Or, move-constructor:
        //
        // create a temporary A22
        //    A::A(int) this 0x1ffefffbd8 data{0x61a9c80:22}
        // create a pair entry, stealing data from the temporary:
        //    A::A(A&&) this 0x1ffefffc60 data{0x61a9c80:22} other 0x1ffefffbd8 data{0:0}
        // create a map entry, stealing data from en element in the pair:
        //    A::A(A&&) this 0x61aa150 data{0x61a9c80:22} other 0x1ffefffc60 data{0:0}
        amap.insert(std::make_pair(std::string{"key1"}, A{22}));

        for (const auto &m : amap) {
            std::cout << m.first << m.second << '\n';
        }

        return 0;
    }
    ```
- How to search by more than one key?
    ```
    struct Element {
        std::string name;
        std::string color;
        int priority;
    };

    // bool operator<(const Element &rhs) const;
    bool Element::operator<(const Element &rhs) const
    {
        return priority < rhs.priority;
    }

    struct ComparePrio {
        using is_transparent = void;

        bool operator()(const Element &e1, const Element &e2) const { return e1.priority < e2.priority; }
        bool operator()(int prio, const Element &e2) const { return prio < e2.priority; }
        bool operator()(const Element &e1, int prio) const { return e1.priority < prio; }
    };

    struct CompareColor {
        using is_transparent = void;

        bool operator()(const ElementPtr &e1, const ElementPtr &e2) const { return e1->color < e2->color; }
        bool operator()(const std::string &color, const ElementPtr &e2) const { return color < e2->color; }
        bool operator()(const ElementPtr &e1, const std::string &color) const { return e1->color < color; }
    };

    int main()
    {
        std::set<Element, ComparePrio> list {
            Element{"orange", "orange", 50},
            Element{"banana", "yellow", 30},
            Element{"apple", "red", 40},
            Element{"topmost", "white", 10},
        };

        std::set<ElementPtr, CompareColor> by_color;
        std::set<ElementPtr, CompareName> by_name;

        for (const auto &e : list) {
            by_color.emplace(&e);
            by_name.emplace(&e);
        }

        // Search an element by priority
        constexpr int prio = 30;
        const auto i = list.find(prio);
        if (i != list.end()) {
            std::cout << "found " << i->priority << " " << i->color << " " << i->name << '\n';
        } else {
            std::cout << "not found " << prio << '\n';
        }

        // Search through the elements by name
        const std::string color = "yellow";
        const auto j = by_color.find(color);
        if (j != by_color.end()) {
            std::cout << "found " << (*j)->priority << " " << (*j)->color << " " << (*j)->name << '\n';
        } else {
            std::cout << "not found " << color << '\n';
        }
    }
    ```
- How to concatenate a std::string and an int?
    ```c++
    std::string name = "John";
    int age = 21;
    ```
    How do I combine them to get a single string "John21"? [1]<br>
    [1] https://stackoverflow.com/questions/191757/how-to-concatenate-a-stdstring-and-an-int
- delete a node from a list

    ```c++
    // Function to delete a node without any reference to head pointer.
    // struct Node {
    //     Node *next;
    //     int data;
    // };
    // List example: 33 -> 44 -> 13 -> 17 -> 53
    void delete_node(Node *del)
    {
        if (!del) {
            return;
        }

        Node *tmp_next = del->next;
        if (!tmp_next) {
            return;
        }

        *del = *del->next;
        //del->next = tmp_next->next;
        //del->data = tmp_next->data;
        delete tmp_next;
    }
    ```
- Call pure virtual method
    ```c++
    /*
     * $ g++ -Wall -Wextra  -O2 -g -std=c++11  -Wl,--build-id=none  test_call_pure.cc   -o test_call_pure
     *
     * $ ./test_call_pure
     * pure virtual method called
     * terminate called without an active exception
     * Aborted (core dumped)
     *
     */

    #include <iostream>

    struct Base {
        Base();
        void init(int a);
        virtual bool foo(int a) = 0;
    };

    struct Derived: public Base {
        bool foo(int a) override;
    };

    Base::Base()
    {
        init(0);
    }

    void Base::init(int a)
    {
        foo(a);
    }

    bool Derived::foo(int a)
    {
        return a > 0;
    }

    int main()
    {
        Derived d;
        return 0;
    }
    ```

- Find minimal distance between 'X' and 'Y'
    ```c++
    /*
     *  dist("XX")  =>  0
     *  dist("YY")  =>  0
     *  dist("XY")  =>  1
     *  dist("YX")  =>  1
     *  dist("OOO")  =>  0
     *  dist("OOYOY")  =>  0
     *  dist("OOYOXOO")   =>  2
     *  dist("OOYOXOXYO")  =>  1
     *  dist("OOYOXOOOOXYO")  =>  1
     *  dist("OOYOXOOOXOOOXYX")  =>  1
     *  dist("OOYOXOXYO")  =>  0
     */

    int dist(const char *s)
    {
        int min1 = INT_MAX;
        const char *start = nullptr;
        while (*s) {
            switch (*s) {
            case 'X':
            case 'Y':
                if (start && *start != *s) {
                    min1 = std::min(min1, static_cast<int>(s - start));
                }
                start = s;
                break;
            case 'O':
                break;
            default:
                return 0;
            }

            s++;
        }

        if (min1 == INT_MAX) {
            return 0;
        }

        return min1;
    }

    ```
- Find an element in sorted array
    ```c++
    int binarysearch(const int arr[], int n, int k)
    {
            if (!n) {
                return -1;
            }

            if (n == 1) {
                if (arr[0] == k) {
                    return 0;
                } else {
                    return -1;
                }
            }

            int n1 = n/2 + (n & 1);
            int n2 = n/2;
            int idx = n1 - 1;
            int el = arr[idx];
            if (k < el) {
                idx = binarysearch(arr, n1, k);
                if (idx == -1) {
                    return -1;
                }

                return idx;
            } else if (k > el) {
                idx = binarysearch(arr + n1, n2, k);
                if (idx == -1) {
                    return -1;
                }

                return n1 + idx;
            }

            return idx;
    }
    ```
- C++ class virtual table (vtable) and virtual table pointer (vpointer or vptr):
  what are these, how and where are they stored?

  - [How are virtual functions and vtable implemented?](https://stackoverflow.com/questions/99297/how-are-virtual-functions-and-vtable-implemented)

  - [Undefined reference to vtable](https://stackoverflow.com/questions/3065154/undefined-reference-to-vtable)

  - [Abstract Class vs Interface in C++ [duplicate]](https://stackoverflow.com/questions/12854778/abstract-class-vs-interface-in-c)

  - [How do you declare an interface in C++?](https://stackoverflow.com/questions/318064/how-do-you-declare-an-interface-in-c)

  - [Why a pure virtual destructor needs an implementation](https://stackoverflow.com/questions/21109417/why-a-pure-virtual-destructor-needs-an-implementation), and in particular this
    [answer](https://stackoverflow.com/a/21110040):
    ```c++
    struct Base
    {
       virtual ~Base()    = 0;  // invoked no matter what
       virtual void foo() = 0;  // only invoked if `Base::foo()` is called
    };

    Base::~Base() {}
    /* void Base::foo() {} */

    struct Derived : Base
    {
       virtual void foo() { /* Base::foo(); */ }
    };

    int main()
    {
        std::unique_ptr<Base> ptr(new Derived());
    }
    ```

- Networking
  - Server: `socket`, `bind`, `listen`, `accept` or `select`/`poll`,
    `read`/`write` or `recv`/`send`
  - Client: `socket`, `connect`, write`/`read` or `send`/`recv`
- Задан входной файл, содержащий целые числа по одному на строку.  Необходимо
  отсортировать их и записать в выходной файл в том же формате.  Можно считать,
  что содержимое файла поместится в оперативную память.
  - C syscalls (open/read/write/close)
  - C libc (fopen/getline/fclose)
  - C++ STL
  - C++ Qt
- `select` vs `poll` vs `epoll` (See
  ["Select v.s. poll v.s. epoll"](https://hechao.li/2022/01/04/select-vs-poll-vs-epoll/) [1])
- Who is the owner of the a `/tmp/sda-uuid.txt` file and why?
    ```
    [eremin@home-desktop ~]$ id
    uid=1000(eremin) gid=100(users) groups=100(users)
    [eremin@home-desktop ~]$ sudo blkid /dev/sda > /tmp/sda-uuid.txt
    ```

- `checkLoginTime`
    ```
    // "00:00-01:00"
    static bool checkLoginTime(const std::string &sched_time)
    {
    }
    ```

## References

 1. Select v.s. poll v.s. epoll  
    https://hechao.li/2022/01/04/select-vs-poll-vs-epoll/

 1. GotW #5 Solution: Overriding Virtual Functions  
    https://herbsutter.com/2013/05/22/gotw-5-solution-overriding-virtual-functions/

 1. When to use virtual destructors?  
    https://stackoverflow.com/questions/461203/when-to-use-virtual-destructors

 1. C++ FAQ LITE — Frequently Asked Questions  
    Marshall Cline, <cline@parashift.com>  
    http://www.dietmar-kuehl.de/mirror/c++-faq/index.html

 1. Relocatable vs. Position-Independent Code (or, Virtual Memory isn't Just For Swap)  
    http://davidad.github.io/blog/2014/02/19/relocatable-vs-position-independent-code-or/

 1. pdos.csail.mit.edu: 6.1810: Operating System Engineering<br>
    https://pdos.csail.mit.edu/6.S081/2023/schedule.html

vim:spell:
