<?php

/**
 * @file
 * Admin settings form for commerce Order Cleanup module.
 */

/**
 * Commerce order cleanup settings form.
 **/
function commerce_order_cleanup_settings() {
  $form = array();
  $period_options = array(
    '1 month',
    '2 month',
    '3 month',
    '6 month',
    '9 month',
    '1 year',
    '2 year',
    '3 year',
  );

  $form['commerce_order_cleanup_batch_size'] = array(
    '#type' => 'textfield',
    '#title' => t('Batches size of bulk delete'),
    '#default_value' => variable_get('commerce_order_cleanup_bactch_size', 100),
    '#description' => t('Orders will be deleted as a small batches to avoid timeout.'),
  );

  $form['commerce_order_cleanup_max_date'] = array(
    '#type' => 'select',
    '#title' => t('Allow to delete orders changed before'),
    '#default_value' => variable_get('commerce_order_cleanup_max_date', '1 year'),
    '#options' => array_combine($period_options, $period_options),
    '#description' => t('Allowed to delete orders that are changed up to given period here. It will be dangerous to allow to delete orders changed on current date.'),
  );

  return system_settings_form($form);
}
