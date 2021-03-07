<?php
/**
 * @file
 * Support functions for merging user orders via the usermerge module.
 */

/**
 * Implements hook_usermerge_actions_supported().
 */
function uc_order_usermerge_usermerge_actions_supported() {
  return array(
    'uc_order' => t("Migrate a user's Ubercart orders to the new user."),
  );
}


/**
 * Implements hook_usermerge_account_properties().
 */
function uc_order_usermerge_usermerge_account_properties($user_to_delete, $user_to_keep, $action) {
  $account_properties = array(
    'uc_order' => array(
      'title' => t('Ubercart orders'),
      'items' => array(),
    ),
  );
  return $account_properties;
}


/**
 * Implements hook_usermerge_account_properties_alter().
 */
function uc_order_usermerge_usermerge_account_properties_alter(&$properties, $user_to_delete, $user_to_keep, $action) {
  // Remove uc_order from authored entities because we'll handle it ourselves
  // along with all the other order-related Ubercart tables.
  unset($properties['entities']['items']['uc_order']);
}


/**
 * Implements hook_usermerge_build_review_form_elements().
 */
function uc_order_usermerge_usermerge_build_review_form_elements($review, $account_properties, $user_to_delete, $user_to_keep) {

  $num_orders_to_delete = db_query('
    SELECT COUNT(1)
    FROM {uc_orders}
    WHERE uid = :uid
    ', array(':uid' => $user_to_delete->uid))
    ->fetchField();

  $num_orders_to_keep = db_query('
    SELECT COUNT(1)
    FROM {uc_orders}
    WHERE uid = :uid
    ', array(':uid' => $user_to_keep->uid))
    ->fetchField();

  if (!$num_orders_to_delete && !$num_orders_to_keep) {
    return array();
  }

  $review['uc_order'] = array(
    '#tree' => TRUE,
    '#theme' => 'usermerge_data_review_form_table',
    '#title' => $account_properties['uc_order']['title'],
    '#attributes' => array('no_merge','property_label' => ''),
  );
  $review['uc_order']['orders']['options']['user_to_delete'] = array(
    '#type' => 'markup',
    '#markup' => format_plural($num_orders_to_delete, '1 order to be reassigned', '@count orders to be reassigned'),
  );
  $review['uc_order']['orders']['options']['user_to_keep'] = array(
    '#type' => 'markup',
    '#markup' => format_plural($num_orders_to_keep, '1 order to be maintained', '@count orders to be maintained'),
  );
  return $review;
}


/**
 * Implements hook_usermerge_merge_accounts().
 */
function uc_order_usermerge_usermerge_merge_accounts($user_to_delete, $user_to_keep, $review) {
  $tables = array(
    'uc_orders',
    'uc_order_admin_comments',
    'uc_order_comments',
    'uc_order_log',
  );
  foreach ($tables as $table) {
    db_update($table)
      ->fields(array('uid' => $user_to_keep->uid))
      ->condition('uid', $user_to_delete->uid)
      ->execute();
  }
}
