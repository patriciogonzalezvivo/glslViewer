
#include <cstdio>
#include <string>
#include <iostream>

#include <array>
#include <vector>

#ifndef WIN32
#include <unistd.h>
#endif

#include <lo/lo.h>
#include <lo/lo_cpp.h>

int test1(const char *path, const char *types,
          lo_arg **argv, int argc, lo_message m,
          void *data)
{
    printf("path: %s\n", path);
    printf("types: %s\n", types);
    printf("i: %d\n", argv[0]->i);
    return 0;
}

int test2(lo_arg **argv, int argc)
{
    printf("in test2: %d\n", argv[0]->i);
    return 0;
}

int test3(lo_arg **argv, int argc, lo_message msg)
{
    printf("in test3\n");
    return 0;
}

void init(lo::Server &s)
{
    int j = 234;

    std::cout << "liblo version: " << lo::version() << std::endl;

    lo_timetag now = lo::now();
    std::cout << "now: " << now.sec << "," << now.frac << std::endl;

    lo_timetag imm = lo::immediate();
    std::cout << "immediate: " << imm.sec << "," << imm.frac << std::endl;

    std::cout << "URL: " << s.url() << std::endl;

    class test3
    {
    public:
        test3(int j, std::string s) : _s(s), _j(j) {};
        int operator () (lo_arg **argv, int argc, lo_message msg)
        {
            std::cout << _s << ": " << _j << ", " << argv[0]->i << std::endl;
            return 0;
        }
    private:
        std::string _s;
        int _j;
    };

    s.add_method("test0", "i", test3(j,"test0"));
    s.del_method("test0", "i");

    s.add_method("test1", "i", test1, 0);
    s.add_method("test2", "i", test2);
    s.add_method("test3", "i", test3(j, "test3"));

    s.add_method("test4", "i",
                  [j](lo_arg **argv, int argc)
                  {
                      printf("test4: %d, %d\n", j, argv[0]->i);
                      return 0;
                  });

    j *= 2;
    s.add_method("test5", "i",
                  [j](lo_arg **argv, int argc, lo_message msg)
                  {
                      printf("test5: %d, %d -- ", j, argv[0]->i);
                      lo_message_pp(msg);
                      return 0;
                  });

    j *= 2;
    s.add_method("test6", "i",
                  [j](lo_message msg)
                  {
                      printf("test6: %d -- ", j);
                      lo_message_pp(msg);
                      return 0;
                  });

    j *= 2;
    s.add_method("test7", "i", [j](){printf("test7: %d\n", j); return 0;});
    j *= 2;
    s.add_method("test8", "i", [j](){printf("test8a: %d\n", j);});
    j *= 2;
    s.add_method("test8", "i", [j](){printf("test8b: %d\n", j);});

    j*=2;
    s.add_method("test9", "i", [j](const char *path, const char *types, lo_arg **argv, int argc)
                  {printf("test9.1: %d, %s, %s, %d\n", j, path, types, argv[0]->i); return 1;});
    j*=2;
    s.add_method("test10", "i", [j](const char *types, lo_arg **argv, int argc)
                  {printf("test10.1: %d, %s, %d\n", j, types, argv[0]->i); return 1;});
    j*=2;
    s.add_method("test11", "is", [j](const char *types, lo_arg **argv, int argc, lo_message msg)
                  {printf("test11.1: %d, %s, %d, %s -- ", j, types, argv[0]->i, &argv[1]->s); lo_message_pp(msg); return 1;});

    j*=2;
    s.add_method("test9", "i", [j](const char *path, const char *types, lo_arg **argv, int argc)
                  {printf("test9.2: %d, %s, %s, %d\n", j, path, types, argv[0]->i);});
    j*=2;
    s.add_method("test10", "i", [j](const char *types, lo_arg **argv, int argc)
                  {printf("test10.2: %d, %s, %d\n", j, types, argv[0]->i);});
    j*=2;
    s.add_method("test11", "is", [j](const char *types, lo_arg **argv, int argc, lo_message msg)
                 {printf("test11.2: %d, %s, %d, %s -- ", j, types, argv[0]->i, &argv[1]->s); lo_message_pp(msg);});

    j*=2;
    s.add_method("test12", "i", [j](const lo::Message m)
                 {printf("test12 source: %s\n", m.source().url().c_str());});

    s.add_method(0, 0, [](const char *path, lo_message m){printf("generic: %s ", path); lo_message_pp(m);});

    j*=2;
    s.add_bundle_handlers(
        [j](lo_timetag time){ printf("Bundle start handler! (j=%d)\n", j); },
        [j](){ printf("Bundle end handler! (j=%d)\n", j); } );
}

int main()
{
    int context = 999;
    lo::ServerThread st(9000,
                        [=](int num, const char *msg, const char *where)
                        {printf("error handler: %d\n", context);});
    if (!st.is_valid()) {
        printf("Nope.\n");
        return 1;
    }

    st.set_callbacks([&st](){printf("Thread init: %p.\n",&st); return 0;},
                     [](){printf("Thread cleanup.\n");});

    st.set_callbacks([](){printf("Thread init.\n");},
                     [](){printf("Thread cleanup.\n");});

    std::cout << "URL: " << st.url() << std::endl;

    init(st);

    st.start();

    lo::Address a("localhost", "9000");

    printf("address host %s, port %s\n", a.hostname().c_str(), a.port().c_str());
#ifdef HAVE_GETIFADDRS
    printf("iface: %s\n", a.iface().c_str());
    a.set_iface(std::string(), std::string("127.0.0.1"));
    a.set_iface(0, "127.0.0.1");
    printf("iface: %s\n", a.iface().c_str());
#endif

    a.send_from(st, "test1", "i", 20);
    a.send("test2", "i", 40);
    a.send("test3", "i", 60);
    a.send("test4", "i", 80);
    a.send("test5", "i", 100);
    a.send("test6", "i", 120);
    a.send("test7", "i", 140);
    a.send("test8", "i", 160);
    a.send("test9", "i", 180);
    a.send("test10", std::string("i"), 200);

    lo::Message m;
    m.add("i", 220);
    m.add_string(std::string("blah"));
    a.send("test11", m);

    m.add(lo::Blob(4,"asdf"));
    m.add(lo::Blob(std::vector<char>(5, 'a')));
    m.add(lo::Blob(std::array<char,5>{"asdf"}));
    a.send("blobtest", m);

    a.send(
        lo::Bundle({
                {"test11", lo::Message("is",20,"first in bundle")},
                {"test11", lo::Message("is",30,"second in bundle")}
            })
        );

    lo::Bundle b({{"ok1", lo::Message("is",20,"first in bundle")},
                  lo::Bundle({"ok2", lo::Message("is",30,"second in bundle")})
                 }, LO_TT_IMMEDIATE);
    printf("Bundle:\n"); b.print();

    lo::Bundle::Element e(b.get_element(0));
    printf("Bundle Message: %s ", e.pm.path.c_str()); e.pm.msg.print();

    e = b.get_element(1);
    printf("Bundle Bundle:\n"); e.bundle.print();

    printf("Bundle timestamp: %u.%u\n",
           e.bundle.timestamp().sec,
           e.bundle.timestamp().frac);

    a.send("test12", "i", 240);

    char oscmsg[] = {'/','o','k',0,',','i',0,0,0,0,0,4};
    lo::Message::maybe m2 = lo::Message::deserialise(oscmsg, sizeof(oscmsg));
    if (m2.first == 0) {
        printf("deserialise: %s", oscmsg);
        m2.second.print();
    }
    else {
        printf("Unexpected failure in deserialise(): %d\n", m2.first);
        return 1;
    }

    // Memory for lo_message not copied
    lo::Message m3(m2.second);

    // Memory for lo_message is copied
    lo::Message m4 = m2.second.clone();

#ifdef WIN32
    Sleep(1000);
#else
    sleep(1);
#endif
    printf("%s: %d\n", a.errstr().c_str(), a.get_errno());
    return a.get_errno();
}
