<?php

/**
 * @file
 * Provides Xdebug integration for Drupal.
 *
 * @author Jimmy Berry ("boombatower", http://drupal.org/user/214218)
 */

// If code_coverage GET flag detected or running tests then track code coverage.
if (!empty($_GET['code_coverage']) || !empty($_SERVER['HTTP_X_DRUPAL_TEST_INFO'])) {
  if (!empty($_GET['code_coverage'])) {
    $coverage_set = time();
  }
  else {
    $test_info = json_decode($_SERVER['HTTP_X_DRUPAL_TEST_INFO'], TRUE);
    $coverage_set = $test_info['test_id'];
  }

  code_coverage_start($coverage_set);
}

/**
 * Start keeping track of code coverage.
 *
 * @param $coverage_set
 *   (Optional) The coverage set to process and record data with. If no coverage
 *   set is specified then the usage data will not be recorded, but will be
 *   stored in memory. (defaults to FALSE)
 */
function code_coverage_start($coverage_set = FALSE) {
  // Start Xdebug code coverage and register shutdown function to collect the
  // results and place them in the log table. The bit flags are OR'ed together
  // and lead to unused code lines having an invocation count of -1, and not
  // executed code lines having a count of -2.
  // @see http://www.xdebug.org/docs/code_coverage
  xdebug_start_code_coverage(XDEBUG_CC_DEAD_CODE | XDEBUG_CC_UNUSED);
  if ($coverage_set) drupal_register_shutdown_function('code_coverage_record', $coverage_set, !empty($_GET['code_coverage']));
}

/**
 * Store keeping track of code coverage.
 *
 * @param $cleanup
 *   (Optional) TRUE to destroy the in memory data structure, otherwise FALSE to
 *   stop keeping track, but leave data in memory for continuation or
 *   processing. (defaults to TRUE)
 */
function code_coverage_stop($cleanup = TRUE) {
  xdebug_stop_code_coverage($cleanup);
}

/**
 * Record code coverage results in log table.
 *
 * @param $coverage_set
 *   Coverage set ID.
 * @param $process
 *   (Optional) TRUE to process the log immediately, otherwise FALSE (deaults to
 *   FALSE).
 */
function code_coverage_record($coverage_set, $process = FALSE) {
  // If a test is running then reset the temporary directory so the default
  // system wide directory will be used.
  if (!empty($GLOBALS['drupal_test_info'])) {
    $temp = $GLOBALS['conf']['file_temporary_path'];
    unset($GLOBALS['conf']['file_temporary_path']);
  }

  // Encode the code coverage data as json and store in a temporary file.
  $coverage = xdebug_get_code_coverage();
  code_coverage_stop();
  file_unmanaged_save_data(json_encode($coverage), 'temporary://code_coverage_' . $coverage_set . '.json');

  // Restore the temporary directory to testing value.
  if (!empty($GLOBALS['drupal_test_info'])) $GLOBALS['conf']['file_temporary_path'] = $temp;

  // Process log immediately.
  if ($process) {
    module_load_include('process.inc', 'code_coverage');
    code_coverage_process($coverage_set);
    echo 'View the <a href="code_coverage/' . $coverage_set . '">code coverage report</a> for the page run.';
  }
}
