/**
 * @file action_test.cpp
 * @copyright defined in bitconch/LICENSE
 */
#include <bitconchiolib/action.hpp>
#include <bitconchiolib/chain.h>
#include <bitconchiolib/crypto.h>
#include <bitconchiolib/datastream.hpp>
#include <bitconchiolib/db.h>
#include <bitconchiolib/bitconchio.hpp>
#include <bitconchiolib/print.hpp>
#include <bitconchiolib/privileged.h>
#include <bitconchiolib/transaction.hpp>

#include "test_api.hpp"

using namespace bitconchio;

void test_action::read_action_normal() {

   char buffer[100];
   uint32_t total = 0;

   bitconchio_assert( action_data_size() == sizeof(dummy_action), "action_size() == sizeof(dummy_action)" );

   total = read_action_data( buffer, 30 );
   bitconchio_assert( total == sizeof(dummy_action) , "read_action(30)" );

   total = read_action_data( buffer, 100 );
   bitconchio_assert(total == sizeof(dummy_action) , "read_action(100)" );

   total = read_action_data(buffer, 5);
   bitconchio_assert( total == 5 , "read_action(5)" );

   total = read_action_data(buffer, sizeof(dummy_action) );
   bitconchio_assert( total == sizeof(dummy_action), "read_action(sizeof(dummy_action))" );

   dummy_action *dummy13 = reinterpret_cast<dummy_action *>(buffer);

   bitconchio_assert( dummy13->a == DUMMY_ACTION_DEFAULT_A, "dummy13->a == DUMMY_ACTION_DEFAULT_A" );
   bitconchio_assert( dummy13->b == DUMMY_ACTION_DEFAULT_B, "dummy13->b == DUMMY_ACTION_DEFAULT_B" );
   bitconchio_assert( dummy13->c == DUMMY_ACTION_DEFAULT_C, "dummy13->c == DUMMY_ACTION_DEFAULT_C" );
}

void test_action::test_dummy_action() {
   char buffer[100];
   int total = 0;

   // get_action
   total = get_action( 1, 0, buffer, 0 );
   total = get_action( 1, 0, buffer, static_cast<size_t>(total) );
   bitconchio_assert( total > 0, "get_action failed" );
   bitconchio::action act = bitconchio::get_action( 1, 0 );
   bitconchio_assert( act.authorization.back().actor == "testapi"_n, "incorrect permission actor" );
   bitconchio_assert( act.authorization.back().permission == "active"_n, "incorrect permission name" );
   bitconchio_assert( bitconchio::pack_size(act) == static_cast<size_t>(total), "pack_size does not match get_action size" );
   bitconchio_assert( act.account == "testapi"_n, "expected testapi account" );

   dummy_action dum13 = act.data_as<dummy_action>();

   if ( dum13.b == 200 ) {
      // attempt to access context free only api
      get_context_free_data( 0, nullptr, 0 );
      bitconchio_assert( false, "get_context_free_data() not allowed in non-context free action" );
   } else {
      bitconchio_assert( dum13.a == DUMMY_ACTION_DEFAULT_A, "dum13.a == DUMMY_ACTION_DEFAULT_A" );
      bitconchio_assert( dum13.b == DUMMY_ACTION_DEFAULT_B, "dum13.b == DUMMY_ACTION_DEFAULT_B" );
      bitconchio_assert( dum13.c == DUMMY_ACTION_DEFAULT_C, "dum13.c == DUMMY_ACTION_DEFAULT_C" );
   }
}

void test_action::read_action_to_0() {
   read_action_data( (void *)0, action_data_size() );
}

void test_action::read_action_to_64k() {
   read_action_data( (void *)((1<<16)-2), action_data_size());
}

void test_action::test_cf_action() {

   bitconchio::action act = bitconchio::get_action( 0, 0 );
   cf_action cfa = act.data_as<cf_action>();
   if ( cfa.payload == 100 ) {
      // verify read of get_context_free_data, also verifies system api access
      int size = get_context_free_data( cfa.cfd_idx, nullptr, 0 );
      bitconchio_assert( size > 0, "size determination failed" );
      std::vector<char> cfd( static_cast<size_t>(size) );
      size = get_context_free_data( cfa.cfd_idx, &cfd[0], static_cast<size_t>(size) );
      bitconchio_assert(static_cast<size_t>(size) == cfd.size(), "get_context_free_data failed" );
      uint32_t v = bitconchio::unpack<uint32_t>( &cfd[0], cfd.size() );
      bitconchio_assert( v == cfa.payload, "invalid value" );

      // verify crypto api access
      capi_checksum256 hash;
      char test[] = "test";
      sha256( test, sizeof(test), &hash );
      assert_sha256( test, sizeof(test), &hash );
      // verify action api access
      action_data_size();
      // verify console api access
      bitconchio::print("test\n");
      // verify memory api access
      uint32_t i = 42;
      memccpy( &v, &i, sizeof(i), sizeof(i) );
      // verify transaction api access
      bitconchio_assert(transaction_size() > 0, "transaction_size failed");
      // verify softfloat api access
      float f1 = 1.0f, f2 = 2.0f;
      float f3 = f1 + f2;
      bitconchio_assert( f3 >  2.0f, "Unable to add float.");
      // verify context_free_system_api
      bitconchio_assert( true, "verify bitconchio_assert can be called" );


   } else if ( cfa.payload == 200 ) {
      // attempt to access non context free api, privileged_api
      is_privileged(act.name.value);
      bitconchio_assert( false, "privileged_api should not be allowed" );
   } else if ( cfa.payload == 201 ) {
      // attempt to access non context free api, producer_api
      get_active_producers( nullptr, 0 );
      bitconchio_assert( false, "producer_api should not be allowed" );
   } else if ( cfa.payload == 202 ) {
      // attempt to access non context free api, db_api
      db_store_i64( "testapi"_n.value, "testapi"_n.value, "testapi"_n.value, 0, "test", 4 );
      bitconchio_assert( false, "db_api should not be allowed" );
   } else if ( cfa.payload == 203 ) {
      // attempt to access non context free api, db_api
      uint64_t i = 0;
      db_idx64_store( "testapi"_n.value, "testapi"_n.value, "testapi"_n.value, 0, &i );
      bitconchio_assert( false, "db_api should not be allowed" );
   } else if ( cfa.payload == 204 ) {
      db_find_i64( "testapi"_n.value, "testapi"_n.value, "testapi"_n.value, 1 );
      bitconchio_assert( false, "db_api should not be allowed" );
   } else if ( cfa.payload == 205 ) {
      // attempt to access non context free api, send action
      bitconchio::action dum_act;
      dum_act.send();
      bitconchio_assert( false, "action send should not be allowed" );
   } else if ( cfa.payload == 206 ) {
      bitconchio::require_auth("test"_n);
      bitconchio_assert( false, "authorization_api should not be allowed" );
   } else if ( cfa.payload == 207 ) {
      now();
      bitconchio_assert( false, "system_api should not be allowed" );
   } else if ( cfa.payload == 208 ) {
      current_time();
      bitconchio_assert( false, "system_api should not be allowed" );
   } else if ( cfa.payload == 209 ) {
      publication_time();
      bitconchio_assert( false, "system_api should not be allowed" );
   } else if ( cfa.payload == 210 ) {
      send_inline( (char*)"hello", 6 );
      bitconchio_assert( false, "transaction_api should not be allowed" );
   } else if ( cfa.payload == 211 ) {
      send_deferred( "testapi"_n.value, "testapi"_n.value, "hello", 6, 0 );
      bitconchio_assert( false, "transaction_api should not be allowed" );
   }

}

void test_action::require_notice( uint64_t receiver, uint64_t code, uint64_t action ) {
   (void)code;(void)action;
   if( receiver == "testapi"_n.value ) {
      bitconchio::require_recipient( "acc1"_n );
      bitconchio::require_recipient( "acc2"_n );
      bitconchio::require_recipient( "acc1"_n, "acc2"_n );
      bitconchio_assert( false, "Should've failed" );
   } else if ( receiver == "acc1"_n.value || receiver == "acc2"_n.value ) {
      return;
   }
   bitconchio_assert( false, "Should've failed" );
}

void test_action::require_notice_tests( uint64_t receiver, uint64_t code, uint64_t action ) {
   bitconchio::print( "require_notice_tests" );
   if( receiver == "testapi"_n.value ) {
      bitconchio::print("require_recipient( \"acc5\"_n )");
      bitconchio::require_recipient("acc5"_n);
   } else if( receiver == "acc5"_n.value ) {
      bitconchio::print("require_recipient( \"testapi\"_n )");
      bitconchio::require_recipient("testapi"_n);
   }
}

void test_action::require_auth() {
   prints("require_auth");
   bitconchio::require_auth("acc3"_n);
   bitconchio::require_auth("acc4"_n);
}

void test_action::assert_false() {
   bitconchio_assert( false, "test_action::assert_false" );
}

void test_action::assert_true() {
   bitconchio_assert( true, "test_action::assert_true" );
}

void test_action::assert_true_cf() {
   bitconchio_assert( true, "test_action::assert_true" );
}

void test_action::test_abort() {
   abort();
   bitconchio_assert( false, "should've aborted" );
}

void test_action::test_publication_time() {
   uint64_t pub_time = 0;
   uint32_t total = read_action_data( &pub_time, sizeof(uint64_t) );
   bitconchio_assert( total == sizeof(uint64_t), "total == sizeof(uint64_t)" );
   bitconchio_assert( pub_time == publication_time(), "pub_time == publication_time()" );
}

void test_action::test_current_receiver( uint64_t receiver, uint64_t code, uint64_t action ) {
   (void)code;(void)action;
   name cur_rec;
   read_action_data( &cur_rec, sizeof(name) );

   bitconchio_assert( receiver == cur_rec.value, "the current receiver does not match" );
}

void test_action::test_current_time() {
   uint64_t tmp = 0;
   uint32_t total = read_action_data( &tmp, sizeof(uint64_t) );
   bitconchio_assert( total == sizeof(uint64_t), "total == sizeof(uint64_t)" );
   bitconchio_assert( tmp == current_time(), "tmp == current_time()" );
}

void test_action::test_assert_code() {
   uint64_t code = 0;
   uint32_t total = read_action_data(&code, sizeof(uint64_t));
   bitconchio_assert( total == sizeof(uint64_t), "total == sizeof(uint64_t)" );
   bitconchio_assert_code( false, code );
}

void test_action::test_ram_billing_in_notify( uint64_t receiver, uint64_t code, uint64_t action ) {
   uint128_t tmp = 0;
   uint32_t total = read_action_data( &tmp, sizeof(uint128_t) );
   bitconchio_assert( total == sizeof(uint128_t), "total == sizeof(uint128_t)" );

   uint64_t to_notify = tmp >> 64;
   uint64_t payer = tmp & 0xFFFFFFFFFFFFFFFFULL;

   if( code == receiver ) {
      bitconchio::require_recipient( name{to_notify} );
   } else {
      bitconchio_assert( to_notify == receiver, "notified recipient other than the one specified in to_notify" );

      // Remove main table row if it already exists.
      int itr = db_find_i64( receiver, "notifytest"_n.value, "notifytest"_n.value, "notifytest"_n.value );
      if( itr >= 0 )
         db_remove_i64( itr );

      // Create the main table row simply for the purpose of charging code more RAM.
      if( payer != 0 )
         db_store_i64( "notifytest"_n.value, "notifytest"_n.value, payer, "notifytest"_n.value, &to_notify, sizeof(to_notify) );
   }
}

void test_action::test_action_ordinal1(uint64_t receiver, uint64_t code, uint64_t action) {
   uint64_t _self = receiver;
   if (receiver == "testapi"_n.value) {
      print("exec 1");
      bitconchio::require_recipient( "bob"_n ); //-> exec 2 which would then cause execution of 4, 10

      bitconchio::action act1({name(_self), "active"_n}, name(_self),
                         name(WASM_TEST_ACTION("test_action", "test_action_ordinal2")),
                         std::tuple<>());
      act1.send(); // -> exec 5 which would then cause execution of 6, 7, 8

      if (is_account("fail1"_n)) {
         bitconchio_assert(false, "fail at point 1");
      }

      bitconchio::action act2({name(_self), "active"_n}, name(_self),
                         name(WASM_TEST_ACTION("test_action", "test_action_ordinal3")),
                         std::tuple<>());
      act2.send(); // -> exec 9

      bitconchio::require_recipient( "charlie"_n ); // -> exec 3 which would then cause execution of 11

   } else if (receiver == "bob"_n.value) {
      print("exec 2");
      bitconchio::action act1({name(_self), "active"_n}, name(_self),
                         name(WASM_TEST_ACTION("test_action", "test_action_ordinal_foo")),
                         std::tuple<>());
      act1.send(); // -> exec 10

      bitconchio::require_recipient( "david"_n );  // -> exec 4
   } else if (receiver == "charlie"_n.value) {
      print("exec 3");
      bitconchio::action act1({name(_self), "active"_n}, name(_self),
                         name(WASM_TEST_ACTION("test_action", "test_action_ordinal_bar")),
                         std::tuple<>()); // exec 11
      act1.send();

      if (is_account("fail3"_n)) {
         bitconchio_assert(false, "fail at point 3");
      }

   } else if (receiver == "david"_n.value) {
      print("exec 4");
   } else {
      bitconchio_assert(false, "assert failed at test_action::test_action_ordinal1");
   }
}
void test_action::test_action_ordinal2(uint64_t receiver, uint64_t code, uint64_t action) {
   uint64_t _self = receiver;
   if (receiver == "testapi"_n.value) {
      print("exec 5");
      bitconchio::require_recipient( "david"_n ); // -> exec 6
      bitconchio::require_recipient( "erin"_n ); // -> exec 7

      bitconchio::action act1({name(_self), "active"_n}, name(_self),
                         name(WASM_TEST_ACTION("test_action", "test_action_ordinal4")),
                         std::tuple<>());
      act1.send(); // -> exec 8
   } else if (receiver == "david"_n.value) {
      print("exec 6");
   } else if (receiver == "erin"_n.value) {
      print("exec 7");
   } else {
      bitconchio_assert(false, "assert failed at test_action::test_action_ordinal2");
   }
}
void test_action::test_action_ordinal4(uint64_t receiver, uint64_t code, uint64_t action) {
   print("exec 8");
}
void test_action::test_action_ordinal3(uint64_t receiver, uint64_t code, uint64_t action) {
   print("exec 9");

   if (is_account("failnine"_n)) {
      bitconchio_assert(false, "fail at point 9");
   }
}
void test_action::test_action_ordinal_foo(uint64_t receiver, uint64_t code, uint64_t action) {
   print("exec 10");
}
void test_action::test_action_ordinal_bar(uint64_t receiver, uint64_t code, uint64_t action) {
   print("exec 11");
}
