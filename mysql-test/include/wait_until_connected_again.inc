#
# Include this script to wait until the connection to the
# server has been restored or timeout occurs
#
# $wait_counter - can be set before calling the script, will be reset at the end
#
# $explicit_default_wait_counter - can be set before calling the script,
#                             reset only at the end of the test, overrides $wait_counter
#
# $add_wait_counter - can be set before calling the script,
#                     additional time count to wait during valgrind run.

#
# We require two consequtive connection successes in order to
# work around a race condition on Galera startup where the server
# can temporarily accept queries before starting to reject them again
#

--disable_result_log
--disable_query_log

--let $counter= 1000
if ($explicit_default_wait_counter)
{
  --let $wait_counter= $explicit_default_wait_counter
}
if ($VALGRIND_TEST) {
  --let $wait_counter= 30000
  if ($add_wait_counter) {
    --expr $wait_counter= $wait_counter + $add_wait_counter
  }
}
if ($TSAN_OPTIONS) {
  --let $wait_counter= 15000
}
if ($MTR_MANUAL_DEBUG) {
  --let $counter = 864000
}
if ($wait_counter) {
  --let $counter= $wait_counter
}
--let $mysql_errno= 9999
let $successes= 2;
while ($successes) {
while ($mysql_errno)
{

  # Strangely enough, the server might return "Too many connections"
  # while being shutdown, thus 1040 is an "allowed" error
  # See BUG#36228
  --error 0,1040,1047,1053,1205,2002,2003,2006,2013,1045,ER_CANNOT_FIND_KEY_IN_KEYRING,ER_SECURE_TRANSPORT_REQUIRED,2016,2017
  show session status;
  if ($mysql_errno == 1045){
    --let mysql_errno=0
  }
  if ($mysql_errname == ER_SECURE_TRANSPORT_REQUIRED){
    --let mysql_errno=0
  }
  if ($mysql_errname == ER_CANNOT_FIND_KEY_IN_KEYRING){
    --let mysql_errno=0;
  }
  --dec $counter
  if (!$counter)
  {
    --die Server failed to restart
  }
  --sleep 0.2
}
  --dec $successes
  --sleep 0.1
}

if ($SECONDARY_ENGINE_TEST) {
  --let $secondary_engine_bootstrap = query_get_value($SECONDARY_ENGINE_BOOTSTRAP_QUERY, Value, 1)
  if ( $secondary_engine_bootstrap == ON ) {
    --let $counter = 100
    --let $secondary_engine_status = OFF

    while ($secondary_engine_status == OFF) {
      --let $secondary_engine_status = query_get_value($SECONDARY_ENGINE_STATUS_QUERY, Value, 1)
      --dec $counter

      if (!$counter) {
        --die Secondary engine server is not running or it is not bootstrapped
      }

      --sleep 0.1
    }

    --sleep 0.1

    # Wait till the tables get loaded into secondary engine
    --perl
    my $wait_file= "$ENV{MYSQLTEST_VARDIR}/tmp/wait_until_load";
    while (-e $wait_file) {
      select(undef, undef, undef, (100 / 1000));
    }
    EOF
  }
}

# Reset $wait_counter so that its value won't be used on subsequent
# calls, and default will be used instead.
if (!$explicit_default_wait_counter)
{
  --let $wait_counter= 0
}
--enable_query_log
--enable_result_log
