<?php

/**
 * @file
 * Provides post-processing functions.
 *
 * @author Jimmy Berry ("boombatower", http://drupal.org/user/214218)
 */

/**
 * Process a coverage set of raw Xdebug data.
 *
 * @param $coverage_sets
 *   Coverage set ID. Use 'all' to process all sets.
 *
 * @return
 *   Boolean TRUE if code coverage results for $coverage_set were found and have
 *   been processed, otherwise FALSE.
 */
function code_coverage_process($coverage_sets) {
  $coverage = array();
  if ($coverage_sets === 'all') {
    $coverage_set_query = db_select('code_coverage_sets', 'c')
      ->fields('c', array('id'))
      ->execute()
      ->fetchCol();
    $coverage_sets = array();
    foreach ($coverage_set_query as $set => $id) {
      array_push($coverage_sets, $id);
    }
  }
  foreach ((array)$coverage_sets as $coverage_set) {
    $coverage_files = file_scan_directory('temporary://', '/code_coverage_' . $coverage_set . '.*\.json/', array('recurse' => FALSE));
    foreach ($coverage_files as $coverage_file) {
      // Load, decode, and process a raw set of Xdebug data.
      $data = json_decode(file_get_contents($coverage_file->uri), TRUE);
      $data = code_coverage_process_set($data);

      // Merge new data with overall coverage data.
      foreach ($data as $file => $lines) {
        foreach ($lines as $line => $covered) {
          // If an existing value for a line exists perform an OR condition. Since
          // covered is represented by TRUE if a line is covered in any dataset
          // the end result will be covered.
          $coverage[$file][$line] = isset($coverage[$file][$line]) ? $coverage[$file][$line] || $covered : $covered;
        }
      }

      // Remove raw data file once it has been processed.
      file_unmanaged_delete($coverage_file->uri);
    }

    // Store coverage data.
    foreach ($coverage as $file => $lines) {
      $file_id = db_insert('code_coverage')
        ->fields(array(
          'coverage_set' => $coverage_set,
          'path' => $file,
          'executed' => count(array_filter($lines)),
          'executable' => count($lines),
          'data' => json_encode($lines),
        ))
        ->execute();
    }

    // Get rid of the entry in the database.
    $delete_entry = db_delete('code_coverage_sets')
      ->condition('id', $coverage_set)
      ->execute();
  }

  return (bool) $coverage;
}

/**
 * Process a coverage file and provide sanitized data.
 *
 * @param $data
 *   An array of Xdebug coverage data.
 * @param $check
 *   (Optional) TRUE to check the file path, otherwise FALSE.
 *
 * @return
 *   An array of sanitized coverage data.
 * @see code_coverage_path_check()
 */
function code_coverage_process_set(array $data, $check = TRUE) {
  $coverage = array();
  foreach ($data as $file => $lines) {
    // Record the relative file path only, and normalize file path on Windows.
    // @todo Deal with symbolic links.
    $relative = strtr($file, array(
      DRUPAL_ROOT . DIRECTORY_SEPARATOR => '',
      DIRECTORY_SEPARATOR => '/',
    ));

    // Ensure that the path is not part of code_coverage and passes any filters.
    if (!$check || code_coverage_path_check($file, $relative)) {
      $coverage[$relative] = array();

      foreach ($lines as $line => $state) {
        // Ignore -2 (DEAD_CODE).
        if ($state != -2) {
          $coverage[$relative][$line] = $state == 1; // TRUE if execute, FALSE if not-executed but executable.
        }
      }
    }
  }
  return $coverage;
}

/**
 * Ensure that the path is not part of code_coverage and passes any filters.
 *
 * @param $full
 *   Full file path.
 * @param $relative
 *   Relative file path.
 *
 * @return
 *   TRUE if path passes, otherwise FALSE.
 */
function code_coverage_path_check($full, $relative) {
  static $paths = array();
  $modules = variable_get('code_coverage_modules', array());
  $files = variable_get('code_coverage_files', array());

  // If the path has already been processed then return the result.
  if (isset($paths[$relative])) {
    return $paths[$relative];
  }

  // Ensure that the directory is not the code_coverage directory.
  if (dirname($full) == dirname(__FILE__)) {
    return $paths[$relative] = FALSE;
  }

  // If neither filter is set the approve the path.
  if (empty($modules) && empty($files)) {
    return $paths[$relative] = TRUE;
  }

  // If a module filter is set then approve any paths within the module list.
  foreach ($modules as $module) {
    if (strpos($relative, $module) !== FALSE) {
      return $paths[$relative] = TRUE;
    }
  }

  // If a file filter is set then approve any paths found in the list.
  if (in_array($relative, $files)) {
    return $paths[$relative] = TRUE;
  }
  return $paths[$relative] = FALSE;
}
