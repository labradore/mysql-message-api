# API Functions #
  * ` send_mesg() `
  * ` join_mesg_group() `
  * ` recv_mesg() `
  * ` leave_mesg_group() `
  * ` track_memberships() `
  * ` mesg_handle() `
  * ` mesg_status() `


# Function Reference #

  * `send_mesg`

```
send_mesg("group","message")
```
Returns TRUE if the message was sent succesfully, or NULL on error. (See also the three-argument form of this call, below).

```
SELECT send_mesg("orders", "two eggs over easy") ;
```
Send the message message to all members of group group.

Spread messages a typically multicast, but if group is a private name (rather than a group name) the message is unicast to a single recipient.

```
send_mesg("group","message","group-member")
```
Returns TRUE if the message was sent succesfully to a member whose name matches group-member, or FALSE if group-member was not a member of the group when the message arrived, or NULL in case of error.


```
SELECT send_mesg("orders", "two eggs over easy", "cook") ;
```
The three-argument form of send\_mesg() sends a "guaranteed" message for a specified member. If the group does not have a member whose name matches group\_member, then the function will return false.

In order to send a guaranteed message to a particular group, you must first call track\_memberships() with the group name.

In matching group\_member against a group's membership list, any leading hash mark is ignored, and group\_member is treated as a prefix. For example, the message to "cook" will succeed if Spread will tell you that the "orders" group has a member named "#cook1#host1"
> 
---


  * ` join_mesg_group("group") `

Returns a small positive integer, referred to as a message handle, or NULL on error. (See also the two-argument form of this call, below).
```
SELECT join_mesg_group("orders");
```
Join a message group. The message handle returned by this call can be used later in recv\_mesg() calls, to get messages; and in calls to `mesg_status()`; and in `leave_mesg_group()`.

Once you join a group, the Spread daemon on your local machine begins queueing messages for you. You must try to receive them.

```
join_mesg_group("group","name")
```
Returns a small positive integer, referred to as a message handle, or NULL on error.
```
SELECT join_mesg_group("orders","cook");
```
The two-argument form of join\_mesg\_group() allows you to join "group" with a particular name. (When you use the one-argument form, your name is chosen randomly).

A name supplied here will match the same name when supplied as the group\_member argument to send\_mesg() (though it may not exactly match the private name used internally by Spread).

> 
---

  * `recv_mesg(handle)`

Returns a string containing a message, or NULL in case of an exception.
```
SELECT recv_mesg(9);
```
A call to `recv_mesg() ` will return the next available message on handle. If no message has been queued for delivery, recv\_mesg() will block until one arrives.

It is often effective to call ` recv_mesg() ` from within SQL stored procedure code, inside a loop.

> 
---

  * `track_memberships("group")`
Returns a small integer message handle, or NULL in case of error.
` SELECT track_memberships("orders"); `
A call to track\_memberships() causes MySQL to pay attention to the the membership list of group, making it possible to then send guaranteed messages to group from this server using the three-argument form of `send_mesg()`.

The message handle returned from the track\_memberships() call can be passed to `mesg_status()`, allowing you to view a group's membership list and other statistics. It can also be passed to `leave_mesg_group()`, which will cause membership tracking for group to end.

> 
---

  * `leave_mesg_group(handle)`

Returns TRUE on success or FALSE on failure.
` SELECT leave_mesg_group(9); `
The `leave_mesg_group()` call takes an integer handle which was created by calling either `join_mesg_group()` or `track_memberships()`.

If the message handle was created by `track_memberships()`, membership tracking for the group will cease.

If the handle was created by `join_mesg_group()`, this call signals your intent to leave the group. It is important to note that calling `leave_mesg_group()` does not immediately disconnect you from the group. There may be messages already received and being held in the message queue, along with other messages already sent but not yet received at the time of your call to `leave_mesg_group()`. After calling `leave_mesg_group()` on a particular handle, you should continue to call `recv_mesg()` on that handle until `recv_mesg()` returns NULL. The NULL return will indicate that you have received all pending messages and succesfully disconnected.

> 
---

  * `mesg_handle(query)`
Returns Returns a small integer message handle, or NULL if no handle matches the query.
    * ` SELECT mesg_handle("track = orders"); `
    * ` SELECT mesg_handle("join = orders"); `
    * ` SELECT mesg_handle("name = #cook#localhost"); `

```
SELECT mesg_handle("track = orders") into mh;
IF mh IS NULL
THEN SELECT track_memberships("orders") into mh;
END IF;
```

```
SELECT leave_mesg_group(mesg_handle("track = orders"));
```

The `mesg_handle()` call provides a way for SQL code in stored routines or scheduled events to use the message API calls without any advance knowledge of open message handles. The call takes a simple query, as a quoted string, and returns a message handle matching the query or NULL to indicate no match.

For a query "track = groupname", the value returned is the message handle associated with a prior call to ` track_memberships("groupname") `.

For a query "join = groupname", the returned handle is a member of group groupname after a prior call to ` join_mesg_group()`, and can be used in `recv_mesg()`. (If multiple message handles are members of groupname, only one handle will be returned).

For a query "name = privatename", the handle returned has the Spread private name privatename. This can be either the complete private name, or the shorter name as it was supplied in `join_mesg_group()`.

> 
---

  * `mesg_status([handle])`


Returns a block of text describing a particular message handle (or all handles.)
  * `SELECT mesg_status() \G `
  * ` SELECT mesg_status(9) \G`


` mesg_status()` returns a block of text describing the status of a message handle. If no handle is specified, the text describes all handles, as well as some global status information on the message API.

The output includes statistics on the number of messages sent and received, the private names of each connection, the names of all members of any groups being tracked, and other information useful for debugging and performance tuning.

Using the command-line mysql client, the status message is easiest to read if the `mesg_status()` query is terminated with the \G delimiter to format the output vertically.
