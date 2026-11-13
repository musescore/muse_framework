# 00006 Core Architecture: Asynchronous interaction

Date: 2020-05-30   
Tags: async, channel, notification, promise     
Maintainers: Igor Korsukov   

## Status: Accepted

## Context

When developing an application, we often encounter a number of tasks related to asynchronous interaction.   
These include tasks such as:   
* What to do when something has happened    
* Waiting for a result (e.g., a user response dialog, a network request, or the execution of a task on a background thread)   
* To update something when a value, list, etc., changes.   
   
We could directly call the methods responsible for performing actions at the point where the event originates. However, this approach requires frequent modifications to existing objects whenever new consumers are added, leading to semantic and technical dependencies and creating a tangled mess; it becomes impossible to abstract the event source, swap its implementation, or reuse it in other scenarios. Furthermore, when different threads interact, synchronization must be handled; moreover, it is often simply not possible to call a consumer method from another thread.     

## Decision

To avoid all these problems, an approach is used in which the source notifies of a change or sends data to a channel, and consumers can subscribe and perform their actions.     
There can be many different implementations of this approach (listeners, signal/slot); we are choosing an implementation that uses an intermediary - a special object that facilitates interaction between the source and the consumer or rather, several objects for different tasks:   
* Channel - is a primitive for bidirectional transmission of typed data. Inspired by GoLang channel.   
* Notification - is designed to send notifications. They have all the qualities of channels, they are only used without arguments and have slightly different semantics.   
* Promise - is an object representing the eventual completion or failure of an asynchronous operation. Inspired by JS promise.   
* Async::call - execute the operation on the next event loop iteration or within the event loop of a specified thread. This is auxiliary infrastructure.  

## Consequences

* The event source is independent; it does not "know" about its consumers and can be used in various scenarios.  
* We can transmit various types of data.  
* No additional infrastructure code needs to be written in the source or the consumer.  
* When a consumer is deleted, an automatic unsubscribe occurs.   
* In the case of inter-thread interaction, the handler automatically executes in the thread where the subscription was made.  

## Alternatives

Qt’s signals and slots provide similar functionality. However, this entails a dependency on Qt (we use Qt only for the UI), inconvenience when using abstract interfaces, and a number of other issues.   

## Implementation 

If the subscription is created within an object, class, we need to have it inherit from `async::Asyncable` and pass it as the first argument so that auto-disconnection occurs when the object is destroyed. 

### Channel 

Сhannels is a primitive for bidirectional transmission of typed data.

* Can subscribe to the channel and send data
* Channels can be returned from methods
* Channels can be passed to methods
* Can unsubscribe from the channel
* Channel can be closed

#### Can subscribe to the channel and send data

```
Channel<int> ch;
ch.onReceive(nullptr, [](int val) {
    std::cout << "onReceive val: " << val << "\n";
});

ch.send(1); // will be printed: 'onReceive val: 1'
```

#### Channel can be returned from methods

```
struct Counter {
    Channel<int> m_ch;
    int m_val = 0;

    void increment()
    {
        ++m_val;
        m_ch.send(m_val);
    }

    Channel<int> channel() const
    {
        return m_ch;
    }
};


int main(int argc, char* argv[])
{
    Counter counter;

    counter.channel().onReceive(nullptr, [](int val) {
        std::cout << "counter: " << val << "\n";
    });

    for (int i = 0; i < 5; ++i) {
        counter.increment(); // will be printed: 'counter: n'
    }
}
```

#### Channel can be passed to methods   
If the subscription occurs inside the class method, then it is required to inherit from the Asyncable interface and you need to pass this to the subscription method so that there is auto disconnect when deleting an object    
   
```
class Receiver : public async::Asyncable
{
public:
    Receiver(const Channel<int>& ch)
        : m_ch(ch)
    {
        m_ch.onReceive(this, [](int val) {
            std::cout << "Receiver channel: " << val << "\n";
        });
    }

private:
    Channel<int> m_ch;
};

int main(int argc, char* argv[])
{
    Counter counter;
    Channel<int> ch = counter.channel();

    {
        Receiver r(ch);
        for (int i = 0; i < 5; ++i) {
            counter.increment(); // will be printed: 'Receiver channel: n'
        }
    } // When deleting an object, all its callbacks will be unsubscribed.
}
```

#### Channels can be used for inter-thread communication.

```
int main(int argc, char* argv[])
{
    Channel<int> ch;
    ch.onReceive(nullptr, [](int val) {
        std::cout << "onReceive val: " << val << "\n"; // will be printed: 'onReceive val: 42'
    });

    std::thread thread([ch]() {
        //! NOTE Calc and send out data
        ch.send(40 + 2);
    });  

    //! NOTE Main event loop imitation
    while (running) {
        app::async::processEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    thread.join();  
}
```

#### Can disconnect from the channel
  
```
class Receiver : public async::Asyncable
{
...
    void unsubscribe()
    {
        m_ch.disconnect(this);
    }
}

int main(int argc, char* argv[])
{
    Counter counter;
    Channel<int> ch = counter.channel();

    Receiver r(ch);
    for (int i = 0; i < 5; ++i) {
        counter.increment(); // will be printed: 'Receiver channel: n'
    }

    r.unsubscribe();

    for (int i = 0; i < 5; ++i) {
        counter.increment(); // nothing will be printed
    }

}
```  

#### Channel can be closed
Closing the channel notifies subscribers that there will be no data coming from the channel.

```
struct Processor
{
    Channel<int> m_ch;

    Channel<int> channel() const
    {
        return m_ch;
    }

    void process()
    {
        for (int task = 0; task < 5; ++task) {
            int val = doCalculate(task);
            m_ch.send(val);
        }

        m_ch.close();
    }
};

struct Saver : public async::Asyncable
{
    Stream m_stream;
    void saveFromChannel(const Channel<int>& ch)
    {
        m_stream.open();
        ch.onReceive(this, [this](int val) {
            m_stream.write(val);
        });

        ch.onClose(this, [this]() {
            m_stream.close();
        });
    }
};

int main(int argc, char* argv[])
{
    Processor p;
    Saver s;

    s.saveFromChannel(p.channel());

    p.process();
}
```

### Notification

Notification is designed to send notifications. They have all the qualities of channels, they are only used without arguments and have slightly different semantics.

```
// Notification sender
class Selector
{
public:
    void select(int val)
    {
        m_selection = val;
        m_selecttionChanged.notify();
    }

    int selection() const
    {
        return m_selection;
    }

    Notification selecttionChanged() const
    {
        return m_selecttionChanged;
    }

private:
    int m_selection = 0;
    Notification m_selecttionChanged;
};

// Notification receiver
class SelectionPrinter : public async::Asyncable
{
    void setSelector(Selector* s)
    {
        if (m_selector) {
            m_selector->selecttionChanged().disconnect(this); // unsubscribe from previous
        }

        m_selector = s;
        m_selector->selecttionChanged().onNotify(this, [this]() {
            std::cout << "Selection changed, new selection: " << m_selector->selection() << "\n";
        });
    }

private:
    Selector* m_selector = nullptr;
};


int main(int argc, char* argv[])
{
    Selector* sel = new Selector();
    SelectionPrinter* p = new SelectionPrinter();

    p->setSelector(sel);

    sel->select(5); // will be printed: 'Selection changed, new selection: 5'
}
```

### Promise  

Promises are similar to Promises in JS. It is used when we need to return the result or an error from an asynchronous operation - such as displaying a dialog to the user, a background task, a network request, etc.   

```
class Calculator
{
public:
  
    async::Promise<int> calc(int v1, int v2)
    {
        return async::make_promise<int>([v1, v2](auto resolve) {
            return resolve(v1 + v2);
        });
    }
}

int main(int argc, char* argv[])
{
    Calculator c;
    
    c.calc(40, 2).onResolve(nullptr, [](int val) {
        std::cout << "calc val: " << val << "\n"; // will be printed: 'calc val: 42'
    });
    
    //! NOTE Main event loop imitation
    while (running) {
        app::async::processEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}
```

### Async::call  

Execute the operation on the next event loop iteration or within the event loop of a specified thread. This is auxiliary infrastructure. 

```
int main(int argc, char* argv[])
{
    Async::call(nullptr, []() {
        // do something
    });

    //! NOTE Main event loop imitation
    while (running) {
        app::async::processEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}
```