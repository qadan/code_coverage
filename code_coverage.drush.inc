<?php

/**
 * @file
 * Provides drush integration
 *
 * @author Jimmy Berry ("boombatower", http://drupal.org/user/214218)
 */

/**
 * Implements hook_drush_command().
 */
function code_coverage_drush_command() {
  $items = array();

  $items['code-coverage-process'] = array(
    'description' => dt('Process a coverage set of raw Xdebug data.'),
    'arguments' => array(
      'coverage set' => dt('Code coverage set ID. Use "all" to process all coverage sets.'),
    ),
    'aliases' => array('cc-process'),
  );
  $items['code-coverage-export'] = array(
    'description' => dt('Export a code coverage set as a json dump or to html files using --html.'),
    'arguments' => array(
      'coverage set' => dt('Code coverage set ID. Use "latest" to grab the most recent coverage set.'),
    ),
    'options' => array(
      'html' => 'Export line data combined with file contents as formatted HTML files.',
      'path' => 'An optional path to place code coverage files in. If unset, all files will be placed in a folder generated in the Drupal temp directory.',
      'tag-html' => 'Format HTML files as correctly-formatted HTML and not simply drupal-rendered HTML.',
      'generate-index' => 'Generates an index.html file in the root of the HTML export folder.',
      'xml' => 'Generates an XML file full of per-file coverage data.',
      'csv' => 'Generates a CSV file full of total coverage data.',
    ),
    'aliases' => array('cc-export'),
  );
  $items['code-coverage-executable'] = array(
    'description' => dt('Determine the number of executable lines contained in a file. Files are loaded in order to determine lines of code so this should be used for files that are never executed.'),
    'arguments' => array(
      'path' => dt('Path to file.'),
    ),
    'options' => array(
      'full' => 'Export a full coverage entry formatted as json and including: path, executable, and data.',
    ),
    'aliases' => array('cc-executable'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['code-coverage-list-unprocessed'] = array(
    'description' => dt('Dump a list of unprocessed coverage set IDs to stdout.'),
    'options' => array(
      'timestamp' => dt('List each ID with a timestamp of when it was created.'),
      'csv' => dt('Export the list, with timestamps, to a CSV file at at the specified path (defaults to temporary://code_coverage_unprocessed_TIMESTAMP.csv)'),
    ),
    'aliases' => array('cc-list'),
  );

  return $items;
}

/**
 * Validate coverage set ID.
 *
 * @param $coverage_set
 *   A coverage set ID.
 *
 * @return
 *   TRUE if valid, otherwise FALSE.
 */
function drush_code_coverage_process_validate($coverage_set = NULL) {
  if (!$coverage_set) {
    return drush_set_error('COVERAGE_SET', dt('A coverage set ID is required.'));
  }
}

/**
 * Process a coverage set of raw Xdebug data.
 *
 * @param $coverage_set
 *   A coverage set ID.
 */
function drush_code_coverage_process($coverage_set) {
  module_load_include('process.inc', 'code_coverage');
  drush_print(code_coverage_process($coverage_set) ? dt('success') : dt('failure, results may have already been processed'));
}

/**
 * Validate coverage set ID.
 *
 * @param $coverage_set
 *   A coverage set ID.
 *
 * @return
 *   TRUE if valid, otherwise FALSE.
 */
function drush_code_coverage_export_validate($coverage_set = NULL) {
  if (!$coverage_set) {
    return drush_set_error('COVERAGE_SET', dt('A coverage set ID is required.'));
  }

  module_load_include('report.inc', 'code_coverage');
  if (!code_coverage_report_file_list($coverage_set) && $coverage_set !== 'latest') {
    return drush_set_error('COVERAGE_SET', dt('Invalid coverage set ID.'));
  }
}

/**
 * Export a code coverage set to files.
 *
 * @param $coverage_set
 *   A coverage set ID.
 */
function drush_code_coverage_export($coverage_set) {
  // Set the coverage ID in case 'latest' is in use.
  if ($coverage_set === 'latest') {
    $coverage_set = db_select('code_coverage', 'c')
      ->fields('c', array('coverage_set'))
      ->orderBy('coverage_set', 'DESC')
      ->execute()
      ->fetchField();
  }
  // Attempt to create a directory to place exported data.
  if (drush_get_option('path')) {
    $directory = drush_get_option('path');
  }
  else {
    $directory = 'temporary://code_coverage_' . $coverage_set;
  }
  if (!file_prepare_directory($directory, FILE_CREATE_DIRECTORY)) {
    return drush_set_error('DIRECTORY', dt('Failed to create directory.'));
  }
  drupal_mkdir($directory . '/' . $coverage_set);

  if (drush_get_option('html')) {
    // Render the code coverage data for each file as HTML and export to a file.
    $files = code_coverage_report_file_list($coverage_set);
    foreach ($files as $file) {
      $page = code_coverage_report_view_file($coverage_set, $file->file_id);
      $lines = drupal_render(array_pop($page));
      if (drush_get_option('tag-html')) {
        $lines = "<html><head/><body>" . $lines . "</body></html>";
        $lines = str_replace('class="code-coverage-covered"', 'style="color:green"', $lines);
        $lines = str_replace('class="code-coverage-uncovered"', 'style="color:red"', $lines);
      }
      file_unmanaged_save_data($lines, $directory . '/' . $coverage_set . '/' . $file->file_id . '.html', FILE_EXISTS_REPLACE);
    }

    // Save summary data as JSON.
    file_unmanaged_save_data(json_encode($files), $directory . '/summary.json', FILE_EXISTS_REPLACE);
  }
  else {
    $files = db_select('code_coverage', 'c')
      ->fields('c', array('path', 'executed', 'executable', 'data'))
      ->condition('coverage_set', $coverage_set)
      ->orderBy('path')
      ->execute()
      ->fetchAll();
    }
  foreach ($files as &$file) {
    $file->data = json_decode($file->data, TRUE);
  }

  // Generate XML.
  if (drush_get_option('xml')) {
    $files = code_coverage_report_file_list($coverage_set);
    $xml = generate_valid_xml_from_array($files);
    file_unmanaged_save_data($xml, $directory . '/coverage.xml', FILE_EXISTS_REPLACE);
  }

  // Generate CSV.
  if (drush_get_option('csv')) {
    $files = db_select('code_coverage', 'c')
      ->fields('c', array('path', 'executed', 'executable'))
      ->condition('coverage_set', $coverage_set)
      ->orderBy('path')
      ->execute()
      ->fetchAll();

    $csv = "Executable,Executed,Percentage\n";
    $totals = array(
      'executable' => 0,
      'executed' => 0,
      'percentage' => 0,
    );
    $filecount = 0;
    foreach ($files as $file) {
      $file->percentage = $file->executed / $file->executable * 100;
      $totals['executable'] += $file->executable;
      $totals['executed'] += $file->executed;
      $totals['percentage'] += $file->percentage;
      $filecount++;
    }
    $totals['percentage'] = round($totals['percentage'] / $filecount);
    $csv .= implode(',', $totals);

    file_unmanaged_save_data($csv, $directory . '/coverage.csv', FILE_EXISTS_REPLACE);
  }

  file_unmanaged_save_data(json_encode($files), $directory . '/coverage.json', FILE_EXISTS_REPLACE);

  if (drush_get_option('generate-index')) {
    // Create an index.html in the code_coverage directory.
    $index = drupal_render(code_coverage_report_view($coverage_set));
    $index = "<html><head/><body>" . $index . "</body></html>";

    $index_dom = new DOMDocument;
    $index_dom->loadHTML($index);
    $xpath = new DOMXpath($index_dom);
    if (drush_get_option('tag-html')) {
      foreach ($xpath->query('//table') as $node) {
        $node->setAttribute('border', '1');
      }
      foreach ($xpath->query("//tr[@class='code-coverage-covered odd' or @class='code-coverage-covered even']") as $node) {
        $node->removeAttribute('class');
        $node->setAttribute('style', 'color:green');
      }
      foreach ($xpath->query("//tr[@class='code-coverage-uncovered odd' or @class='code-coverage-uncovered even']") as $node) {
        $node->removeAttribute('class');
        $node->setAttribute('style', 'color:red');
      }
    }
    foreach ($xpath->query("//a[@href]") as $node) {
      $href = $node->getAttribute('href');
      $href = str_replace ('/code_coverage/', '', $href);
      $href = $href . ".html";
      $node->setAttribute('href', $href);
    }
    $index = $index_dom->saveHTML();
    file_unmanaged_save_data($index, $directory . '/index.html', FILE_EXISTS_REPLACE);
  }

  // Print directory in which exported data resides.
  drush_print(drupal_realpath($directory));
}

/**
 * Validate the file.
 *
 * @param $file
 *   Path to file.
 */
function drush_code_coverage_executable_validate($file = NULL) {
  if (!$file || !file_exists($file) || !is_file($file)) {
    return drush_set_error('FILE', dt('A valid file is required.'));
  }
}

/**
 * Determine the number of executable lines contained in a file.
 *
 * Files are loaded in order to determine lines of code so this should be used
 * for files that are never executed.
 *
 * @param $path
 *   Path to file.
 */
function drush_code_coverage_executable($path) {
  module_load_include('xdebug.inc', 'code_coverage');
  module_load_include('process.inc', 'code_coverage');

  code_coverage_start();

  // Ensure that any embedded HTML or output generated by the file is hidden.
  ob_start();
  require_once $path;
  ob_end_clean();

  $coverage = xdebug_get_code_coverage();
  code_coverage_stop();

  // Check for coverage data for the specified path.
  if (isset($coverage[$realpath = realpath($path)])) {
    // Process coverage data for single file.
    $coverage = code_coverage_process_set(array($realpath => $coverage[$realpath]), FALSE);

    // If full option is present then print a full coverage description of the
    // file, otherwise print the number of executable lines.
    if (drush_get_option('full')) {
      // Since the file is assumed to be not executed ensure that each line is
      // marked as not covered.
      foreach ($coverage[$path] as $line => &$covered) {
        $covered = FALSE;
      }

      // Build an array of the data to be printed.
      $export = array(
        'path' => $path,
        'executed' => 0,
        'executable' => count($coverage[$path]),
        'data' => $coverage[$path],
      );
      drush_print(json_encode($export));
    }
    else {
      drush_print(count($coverage[$path]));
    }
  }
  else {
    return drush_set_error('NO_COVERAGE', dt('No coverage information found.'));
  }
}

function generate_xml_from_array($array, $node_name) {
  $xml = '';

  if (is_array($array) || is_object($array)) {
    foreach ($array as $key=>$value) {
      if (is_numeric($key)) {
        $key = $node_name;
      }

      $xml .= '<' . $key . '>' . "\n" . generate_xml_from_array($value, $node_name) . '</' . $key . '>' . "\n";
    }
  }
  else {
    $xml = htmlspecialchars($array, ENT_QUOTES) . "\n";
  }

  return $xml;
}

function generate_valid_xml_from_array($array, $node_block='nodes', $node_name='node') {
  $xml = '<?xml version="1.0" encoding="UTF-8" ?>' . "\n";

  $xml .= '<' . $node_block . '>' . "\n";
  $xml .= generate_xml_from_array($array, $node_name);
  $xml .= '</' . $node_block . '>' . "\n";

  return $xml;
}

/**
 * Return a list of unprocessed set IDs.
 */
function drush_code_coverage_list_unprocessed() {
  // Grab the list, check if it's empty.
  $list = db_select('code_coverage_sets', 'c')
      ->fields('c', array('id', 'created'))
      ->execute()
      ->fetchAllKeyed();
  if (empty($list)) {
    drush_print('There are no unprocessed coverage sets.');
    return;
  }

  // If the --csv tag is set, export some CSV.
  if ($path = drush_get_option('csv')) {
    $csv = "ID,Timestamp\n";
    foreach ($list as $id => $created) {
      $csv .= $id . "," . date('Y-m-d h:i:s', $created) . "\n";
    }
    if ($path == 1) {
      $path = 'temporary://code_coverage_unprocessed_' . time() . '.csv';
    }
    file_unmanaged_save_data($csv, $path, FILE_EXISTS_REPLACE);
  }

  //If the --timestamp tag is set, add them in. Otherwise just print the list.
  if (drush_get_option('timestamp')) {
    foreach ($list as $id => $created) {
      drush_print($id . " (" . date('Y-m-d h:i:s', $created) . ")");
    }
  }
  else {
    foreach ($list as $id => $created) {
      drush_print($id);
    }
  }
}
