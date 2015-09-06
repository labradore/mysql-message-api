**wiki:** [Introduction](Introduction.md) | SpreadToolkit | [Examples](Examples.md)

# Spread #

The Message API is built using a high-performance Open Source messaging framework called the Spread Toolkit, which is available from http://www.spread.org/, with commercial support and services available from [Spread Concepts LLC](http://www.spreadconcepts.com). Spread supplies a foundation for a reliable group communications system, and the Message API provides a simple bridge allowing MySQL servers to join that system.

Spread is widely-supported by many Open Source projects and other software components. For Java, there is both a native Spread API and a Spread-based implementation of the standard Java Messaging Service (JMS). Many other languages, including PHP and Perl, have Spread API modules available. The Apache web server can use Spread for logging (with mod\_log\_spread) or to manage server load balancing (with mod\_backhand).

Spread is deployed by running a "Spread daemon" on each server. The applications on a server connect to their local daemon in order to send messages, join groups, and receive messages. Message delivery from a Spread daemon to its local clients is asynchronous, and the daemon will hold each client's unread messages until it is ready to receive them. The message passing among the Spread daemons on a network, however, is tightly controlled and synchronous, providing guaranteed delivery and message ordering.

The Message API depends on Spread in several important ways:

  * The UDFs must be linked against the thread-safe Spread library ( libtspread in Spread 3.17, or libspread in Spread 4.0).
  * The Message APIs require a Spread daemon to be running on each MySQL server.
  * The Spread daemons must be configured, using spread.conf files, to define the domain for group membership.