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
    '2 months',
    '3 months',
    '6 months',
    '9 months',
    '1 year',
    '2 years',
    '3 years',
  );

  $form['commerce_order_cleanup_batch_size'] = array(
    '#type' => 'textfield',
    '#title' => t('Batches size of bulk delete'),
    '#default_value' => variable_get('commerce_order_cleanup_batch_size', 100),
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
