# MySQL Message API #

The MySQL Message API is a set of User-Defined Functions (UDFs) that
enable MySQL database servers to send and receive messages across a
network.  They allow MySQL users to do this using simple SQL queries,
like this one --

```
     SELECT send_mesg("orders", "two eggs over easy") 
```

-- which would send the message "two eggs over easy" to all the members of a
message group named "orders."

A _message group_ is a set whose members might include other MySQL
users on the same server or on other MySQL servers.  Messages might be sent
by a trigger on one database server whenever a particular table is
updated, and processed by a long-running stored procedure on another server.
The membership of a group may also include applications written in PHP, Perl,
Java, or many any other langauages.  The group's members might all be on a
local area network, or could be distributed around the Internet.

The messages are delivered by the Spread Toolkit, a high-performance Open Source
messaging platform that can employ the most efficient possible combination of
IP broadcast, multicast, UDP, and TCP protocols for any network topology.

The delivery of messages is reliable, ordered, and asynchronous.
"Reliable" and "ordered" mean that the message API can guarantee that all
members of a group are able to receive every message sent to that group,
and that all members will receive the messages in the same order.
"Asynchronous" means that message senders need not wait to confirm that each
member has received one message before sending another.  Asynchronous message
transfer allows each member to process messages at its own pace, without missing
any messages, and without slowing down the rest of the group.