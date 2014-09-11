<?php

/**
 * @file
 * Drush integration for the Commerce order cleanup module.
 */

/**
 * Implements hook_drush_help().
 */
function commerce_order_cleanup_drush_help($section) {
  switch ($section) {
    case 'commerce-order-cleanup-delete-orders':
      return dt("This command will delete all commerce orders by various order properties like status, last changed.");
  }
}

/**
 * Implements hook_drush_command().
 */
function commerce_order_cleanup_drush_command() {
  $items = array();
  $date_format = 'd-m-Y h:i:s';
  $default_end_date = date($date_format, strtotime('-' . variable_get('commerce_order_cleanup_max_date', '1 year')));
  $items['commerce-order-cleanup-delete-orders'] = array(
    'description' => dt('Deletes commerce orders by various order properties like status, last changed.'),
    'options' => array(
      'changed' => dt('Order changed timestamp. Use timestamp or time string e.g. "-2 months". '),
      'status' =>  dt('Status of an order. try drush cco --list-options="status" to get available status'),
      'type' =>  dt('Type of line item in an order.'),
    ),
    'examples' => array(
      'drush coc' => dt("Delete all cart orders that were changed on or before @changed_default", array('@changed_default' => $default_end_date)),
      'drush coc --status="checkout_complete"' => dt("Deletes all the orders in status 'checkout_complete' and changed on or before @changed_default", array('@changed_default' => $default_end_date)),
      'drush coc --changed="-2 months"' => dt("Deletes all the orders that are changed on or before 2 months before. Currently you can delete the orders changed between @start_date and @end_date. You can configure the allowed latest date at admin/commerce/config/order-cleanup", array('@start_date' => date($date_format, strtotime('October 15, 2009 9:41pm')), '@end_date' => $default_end_date)),
      'drush coc --changed=1388534400' => dt("Deletes all the orders that are changed on or before 01-01-2014 00:00:00. Currently you can delete the orders changed between @start_date and @end_date. You can configure the allowed latest date at admin/commerce/config/order-cleanup", array('@start_date' => date($date_format, strtotime('October 15, 2009 9:41pm')), '@end_date' => $default_end_date)),
      'drush coc --type="shirt"' => dt("Deletes all the orders that have shirt line item  and changed on or before 01-01-2014 00:00:00. Currently you can delete the orders changed between @start_date and @end_date. You can configure the allowed latest date at admin/commerce/config/order-cleanup", array('@start_date' => date($date_format, strtotime('October 15, 2009 9:41pm')), '@end_date' => $default_end_date)),
    ),
    'aliases' => array('coc'),
  );

  $items['commerce-order-cleanup-list-options'] = array(
    'description' => dt('Lists values available for given order property.'),
    'arguments' => array(
      'name' =>  dt('Property name. e.g. status'),
     ),
    'examples' => array(
      'drush cocl status' => "Provides list of status available.",
    ),
    'aliases' => array('cocl'),
  );

  $items['commerce-order-cleanup-state'] = array(
    'description' => dt('State of orders in the system.'),
    'options' => array(
      'changed' => dt('Order changed timestamp. Use timestamp or time string e.g. "-2 months"'),
      'status' =>  dt('Status of an order. try drush cco --list-options="status" to get available status'),
      'type' =>  dt('Type of line item in an order.'),
    ),
    'examples' => array(
      'drush cocs' => dt("State of all cart orders"),
      'drush cocs --status="checkout_complete"' => dt("State of  all the orders in status 'checkout_complete'"),
    ),
    'aliases' => array('cocs'),
  );

  return $items;
}

/**
 * Deletes orders.
 *
 * @param integer $interval
 *   Duration the orders must have been in an unmodified unfinished state to be
 *   cleaned, in seconds.
 */
function drush_commerce_order_cleanup_delete_orders($interval = 0) {
  $filter = _commerce_order_cleanup_build_query_filter();
  // Query filter can throw errors for invalid data.
  if (drush_get_error()) {
    return;
  }

  $query = "SELECT co.order_id FROM {commerce_order} co $filter";
  $result = db_query($query);
  foreach ($result as $row) {
    $order_ids[] = $row->order_id;
  }
  if (empty($order_ids)) {
    return drush_set_error('No orders to delete for given search. Generated query is: ' . $query);
  }
  // Do the deleting part.
  drush_print(dt("Found !count records to delete", array('!count' => count($order_ids))));
  // Display the state of the records before delete.
  drush_commerce_order_cleanup_state();
  $st_time = microtime(TRUE);
  $batch_size = variable_get('commerce_order_cleanup_batch_size', 100);
  $count = count($order_ids);
  $ids = array_chunk($order_ids, $batch_size);
  $params = array('!total_count' => $count);
  for($i=0; $i< ($count/$batch_size); $i++) {
    $st_del_time = microtime(TRUE);
    try {
     commerce_order_delete($ids[$i]);
    }
    catch(Exception $e) {
      drush_print($e->getMessage());
    }

    $params['!count'] = ($i+1) * $batch_size;
    $params['!seconds'] = microtime(TRUE) - $st_del_time;
    drush_print(dt('Deleted !count of !total_count orders (took !seconds sec).', $params));
  }
  $params['!second'] = microtime(TRUE) - $st_time;
  drush_print(dt('Deleted !total_count orders in !seconds sec.', $params));
}

/**
 * provides list of values available for given property.
 *
 * @param $property
 */
function drush_commerce_order_cleanup_list_options($property) {
  // For now, only property that has this option is 'status'.
  $allowed_properties = array('status');
  if (!empty($property)) {
    if (in_array($property, $allowed_properties)) {
      $list = _commerce_order_cleanup_get_options($property);
      drush_print_table($list, TRUE);
      return;
    }
    else {
      drush_print(dt('There is no option available for given order property.'), 'error');
      return;
    }
  }
}

function drush_commerce_order_cleanup_state() {
  $where = _commerce_order_cleanup_build_query_filter();
  // Query filter can throw errors for invalid data.
  if (drush_get_error()) {
    return;
  }
  $query = "SELECT co.status, COUNT(1) AS count, ci.type FROM {commerce_order} co $where GROUP BY status";
  $result = db_query($query);
  foreach ($result as $row) {
    $status_list[] = array($row->status, $row->type, $row->count);
  }
  if (!empty($status_list)) {
    array_unshift($status_list, array('Status', 'Line item type', 'Count'));
    drush_print_table($status_list, TRUE);
  }
  else {
    return drush_set_error('No orders to list for given search. sql is: ' . $query);
  }
}

function _commerce_order_cleanup_build_query_filter() {
  $where = '';

  // Lineitem type.
  $join = ' JOIN {commerce_line_item} ci ON co.order_id=ci.order_id ';
  if ($type = drush_get_option('type')) {
    if ($type !== NULL) {
      $where .="ci.type='$type'";
    }
    else{
      return drush_set_error('Invalid changed value');
    }
  }

  // User the changed date provided by user if available. Otherwise default.
  $changed_raw = drush_get_option('changed') ? drush_get_option('changed') : '-' . variable_get('commerce_order_cleanup_max_date', '1 year');
  $changed = _commerce_order_cleanup_calculate_changed($changed_raw);
  if ($changed !== NULL) {
    $where .= $where ? ' AND ' : '';
    $where .="co.changed < '$changed'";
  }
  else{
    return drush_set_error('Invalid changed value');
  }

  // Status
  if ($status = drush_get_option('status')) {
    // @TODO: Check if the status exist.
    if ($status !== NULL) {
      $where .= $where ? ' AND ' : '';
      $where .="co.status='$status'";
    }
    else{
      return drush_set_error('Invalid status value');
    }
  }

  if (!empty($where)) {
    $where = $join . 'WHERE ' . $where;
  }
  return $where;
}

/**
 * Providers available order status.
 *
 * @param string $type
 *   A string of type. Allowed value is status for now.
 *
 * @return array
 */
function _commerce_order_cleanup_get_options($type) {
  switch ($type) {
    case 'status':
      // @TODO: fix cache handler, currently throws fatal.
      // $cache_key = 'commerce_order_cleanup:options_status';
      // $status_list = cache_get($cache_key, array());
      if (empty($status_list)) {
        $result = db_query("SELECT status FROM {commerce_order} GROUP BY status");
        foreach ($result as $row) {
          $status_list[] = array($row->status);
        }
        if (!empty($status_list)) {
          array_unshift($status_list, array('Available status'));
        }
        // cache_set($cache_key, $status_list);
      }
      return $status_list;
  }
}


function _commerce_order_cleanup_calculate_changed($changed) {
  if (is_int($changed) && _commerce_order_cleanup_is_valid_date($changed)) {
    return $changed;
  }
  elseif (is_string($changed) && _commerce_order_cleanup_is_valid_date(strtotime($changed))) {
    return strtotime($changed);
  }
  return NULL;
}

/**
 * Validates given date range is valid.
 *
 * @param $timestamp
 *
 * @return bool
 */
function _commerce_order_cleanup_is_valid_date($timestamp) {
  // if it is timestamp, make sure it is in a range.
  // Start of commerce module in drupal.org
  $start_date = strtotime('October 15, 2009 9:41pm');
  $end_date = strtotime('-' . variable_get('commerce_order_cleanup_max_date', '1 year'));
  if ($timestamp < $start_date || $timestamp > $end_date) {
    return FALSE;
  }
  return TRUE;
}
