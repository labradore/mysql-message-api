The MySQL Message API is a set of User-Defined Functions (UDFs) that enable MySQL database servers to send and receive messages across a network. They allow MySQL users to do this using simple SQL queries, like this one

> SELECT send\_mesg('orders', 'two eggs over easy')

which would send the message 'two eggs over easy' to all the members of a message group named 'orders.'

A message group is a set whose members might include other MySQL users on the same server or on other MySQL servers. Messages might be sent by a trigger on one database server whenever a particular table is updated, and processed by a long-running stored procedure on another server. The membership of a group may also include applications written in PHP, Perl, Java, or many any other langauages. The group's members might all be on a local area network, or could be distributed around the Internet.

The messages are delivered by the Spread Toolkit, a high-performance Open Source messaging platform that can employ the most efficient possible combination of IP broadcast, multicast, UDP, and TCP protocols for any network topology.


> --  This product uses software developed by Spread Concepts LLC for use in the Spread toolkit. For more information about Spread see http://www.spread.org. --