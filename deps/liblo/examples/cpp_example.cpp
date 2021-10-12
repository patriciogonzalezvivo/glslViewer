
#include <iostream>
#include <atomic>

#ifndef WIN32
#include <unistd.h>
#endif

#include <lo/lo.h>
#include <lo/lo_cpp.h>

int main()
{
    /*
     * Create a server on a background thread.  Note, all message
     * handlers will run on the background thread!
     */
    lo::ServerThread st(9000);
    if (!st.is_valid()) {
        std::cout << "Nope." << std::endl;
        return 1;
    }

    /* Set some lambdas to be called when the thread starts and
     * ends. Here we demonstrate capturing a reference to the server
     * thread. */
    st.set_callbacks([&st](){printf("Thread init: %p.\n",&st);},
                     [](){printf("Thread cleanup.\n");});

    std::cout << "URL: " << st.url() << std::endl;

    /*
     * Counter for number of messages received -- we use an atomic
     * because it will be updated in a background thread.
     */
    std::atomic<int> received(0);

    /*
     * Add a method handler for "/example,i" using a C++11 lambda to
     * keep it succinct.  We capture a reference to the `received'
     * count and modify it atomatically.
     *
     * You can also pass in a normal function, or a callable function
     * object.
     *
     * Note: If the lambda doesn't specify a return value, the default
     *       is `return 0', meaning "this message has been handled,
     *       don't continue calling the method chain."  If this is not
     *       the desired behaviour, add `return 1' to your method
     *       handlers.
     */
    st.add_method("example", "i",
                  [&received](lo_arg **argv, int)
                  {std::cout << "example (" << (++received) << "): "
                             << argv[0]->i << std::endl;});

    /*
     * Start the server.
     */
    st.start();

    /*
     * Send some messages to the server we just created on localhost.
     */
    lo::Address a("localhost", "9000");

    /*
     * An individual message
     */
    a.send("example", "i", 7890987);

    /*
     * Initalizer lists and message constructors are supported, so
     * that bundles can be created easily:
     */
    a.send(lo::Bundle({{"example", lo::Message("i", 1234321)},
                       {"example", lo::Message("i", 4321234)}}));

    /*
     * Polymorphic overloads on lo::Message::add() mean you don't need
     * to specify the type explicitly.  This is intended to be useful
     * for templates.
     */
    lo::Message m;
    m.add(7654321);
    a.send("example", m);

    /*
     * Wait for messages to be received and processed.
     */
    int tries = 200;
    while (received < 4 && --tries > 0) {
#ifdef WIN32
        Sleep(10);
#else
        usleep(10*1000);
#endif
    }

    if (tries <= 0) {
        std::cout << "Error, waited too long for messages." << std::endl;
        return 1;
    }

    /*
     * Resources are freed automatically, RAII-style, including
     * closing the background server.
     */
    std::cout << "Success!" << std::endl;
}
