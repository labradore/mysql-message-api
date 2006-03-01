

CREATE FUNCTION send_mesg RETURNS INTEGER soname 'message_api.so' ;
CREATE FUNCTION join_mesg_group RETURNS INTEGER soname 'message_api.so' ;
CREATE FUNCTION recv_mesg returns STRING soname 'message_api.so' ;
CREATE FUNCTION leave_mesg_groups RETURNS INTEGER soname 'message_api.so' ;
CREATE FUNCTION track_memberships RETURNS INTEGER soname 'message_api.so' ;
CREATE FUNCTION mesg_status RETURNS STRING soname 'message_api.so';

