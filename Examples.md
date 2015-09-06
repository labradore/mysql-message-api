**Wiki:** [Introduction](Introduction.md) | [SpreadToolkit](SpreadToolkit.md) | [Examples](Examples.md)


# Examples #

## Example 1: Sending Notifications ##

![http://messagequeue.lenoxway.net/mesg-ex1.jpg](http://messagequeue.lenoxway.net/mesg-ex1.jpg)

The database server and all of the applications are members of a Spread segment, and the applications have joined a group called "inserts." Whenever any client creates a new row in the table `xyz_table`, the _notify_ trigger fires, sending a message to all of the applications.


---



## Example 2: Large Web Application: database servers that send _and_ receive ##

![http://messagequeue.lenoxway.net/mesg-ex2.jpg](http://messagequeue.lenoxway.net/mesg-ex2.jpg)

At a large social networking web site, one server holds a "links" table that records links between users (a.k.a. "friends lists"). Another set of servers contains detailed membership records for each user; this data set is partitioned across many database servers, with one server for every million users. Every user record includes a "friend count" that records how many other people a person is linked to.

We can use the message API to maintain the friend count. On the Friends database server, a SQL stored procedure called `send_link_count()` publishes a user's friend count to a message group named "`user_links`."

```
CREATE PROCEDURE send_link_count(user_id INT)
 BEGIN
  DECLARE i INT;
  SELECT send_mesg("user_links",concat(user_id," ",count(*))) 
    INTO i FROM links
    WHERE link_from = user_id;
 END
```

A set of triggers on the links table is used publish updated link counts whenever the table changes. The INSERT trigger is shown here, and there could be a DELETE trigger as well.

```
CREATE TRIGGER send_count
 AFTER INSERT ON links
 FOR EACH ROW
  CALL send_link_count(new.link_from);
```
The `send_link_count()` procedure above used concat() to put together a two-part message containing both the user-id and number of friends. A simple procedure on the user database servers unpacks the two-part message.

```
CREATE PROCEDURE unpack_links_message(
  IN message VARCHAR(50),
  OUT user_id INT,
  OUT link_count INT
)
 BEGIN 
  DECLARE p int;
  SELECT POSITION(" " IN message) INTO p;
  SELECT SUBSTR(message,1,p) INTO user_id;
  SELECT SUBSTR(message,p+1) INTO link_count;
 END
```
Also on the user database servers, the long-running stored procedure `links_listener()` joins the message group, parses each message, and updates the user table.

```
CREATE PROCEDURE links_listener()
 BEGIN
  DECLARE handle, user_id, link_count, p INT;
  DECLARE message VARCHAR(50);
  
  SELECT join_mesg_group("user_links","listener") INTO handle;
  IF handle IS NULL then
    SELECT "Could not connect" as status;
  END IF;
  REPEAT 
    SELECT recv_mesg(handle) INTO message;
    IF message IS NOT NULL then
      CALL unpack_links_message(message,user_id,link_count);
      UPDATE users
       SET friend_count = link_count
       WHERE id = user_id ;
      COMMIT;
    END IF;
  UNTIL message IS NULL
  END REPEAT;
 END;
```
The listener will run until you kill it from another thread.

```
CREATE PROCEDURE kill_listener() 
 BEGIN 
  SELECT leave_mesg_group(mesg_handle("name=listener"));
 END
```


---



## Example 3: XML ##
The message sent in example 2 above is encoded in a very simple format, where the user-id and link count are separated by a space. This format is efficient, but not really robust.

If something compels you to send XML messages, then MySQL 5.1 provides an _ExtractValue_ function which can unpack the XML messages using XPath queries. _ExtractValue_ first appeared in MySQL 5.1.5, so this is not available in MySQL 5.0. I will illustrate the possibilities using a simple one-tag XML message format.

```
  <LinkCount user="15" count="8" />
```
To build the message, use a function `pack_link_count_XML` on the links database server. This involves nothing more than assembling a string, so concat() does all the real work.

```
CREATE FUNCTION pack_link_count_XML(user_id INT, link_count INT) 
RETURNS VARCHAR(100)
DETERMINISTIC NO SQL 
 BEGIN
  RETURN concat(
    "<LinkCount ",
     'user="' , user_id , '" ',
     'count="', link_count , '"', 
    " />" );
 END
```

Then, on the user database servers (the receiving end), the `unpack_links_message` procedure above can be replaced with one that relies on ExtractValue to unpack the XML. This requires two XPath queries, "`/LinkCount/@user`" to get the user id (i.e. the "user" attribute of the tag) and "`/LinkCount/@count`" to get the link count.

```
CREATE PROCEDURE unpack_link_count_XML(
   IN message VARCHAR(100),
   OUT user_id INT,
   OUT link_count INT
)
BEGIN 
  SET user_id = ExtractValue(message,"/LinkCount/@user");
  SET link_count = ExtractValue(message,"/LinkCount/@count");
END
```