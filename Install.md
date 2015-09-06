# Installing the Message API #


### Install MySQL ###
  * The `mysql_config` script and MySQL's C and C++ header files are required for building the Message API.

### Install and configure Spread ###
  * It's available from http://www.spread.org/.  Before using the Message API, you should have a working _spread.conf_ file and running Spread daemons, and you should be able to use the _spuser_ utility to send a test message.

### Download and unpack the Message API source distribution ###
  * from the "Downloads" page here.  Binary distributions are also available for some platforms.

### Run "configure", "make", and "make install" ###
  * This will create a file called `message_api.so` and install it in /usr/local/lib/.  (However, note that some Linux distributions, which do not use the GNU dynamic linker, require you to move the file from /usr/local/lib/ into /usr/lib)

### Create the SQL functions ###
  * Execute the `create_functions.sql` script in your MySQL server
    * `CREATE FUNCTION send_mesg RETURNS INTEGER soname 'message_api.so'; `
    * `CREATE FUNCTION join_mesg_group RETURNS INTEGER soname 'message_api.so';`
    * `CREATE FUNCTION recv_mesg RETURNS STRING soname 'message_api.so';`
    * `CREATE FUNCTION leave_mesg_group RETURNS INTEGER soname 'message_api.so';`
    * `CREATE FUNCTION track_memberships RETURNS INTEGER soname 'message_api.so';`
    * `CREATE FUNCTION mesg_handle RETURNS INT soname 'message_api.so';`
    * `CREATE FUNCTION mesg_status RETURNS STRING soname 'message_api.so';`








