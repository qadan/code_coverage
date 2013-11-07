<?php

/**
 * @file
 * Provides reporting functionality.
 *
 * @author Jimmy Berry ("boombatower", http://drupal.org/user/214218)
 */

/**
 * The minimum percentage that must be reached to be considered covered.
 *
 * @var integer
 */
define('CODE_COVERAGE_COVERED', 90);

/**
 * View the summary report with an index of included files.
 *
 * @param $coverage_set
 *   Coverage set ID.
 * @return
 *   Either a renderable array or nothing.
 */
function code_coverage_report_view($coverage_set) {
  if ($files = code_coverage_report_file_list($coverage_set)) {
    drupal_add_css(drupal_get_path('module', 'code_coverage') . '/code_coverage.css');

    $table = array(
      '#theme' => 'table',
      '#header' => array(t('File'), t('Executed'), t('Executable'), t('Coverage')),
      '#rows' => array(),
      '#attributes' => array('class' => array('code-coverage')),
    );
    $executed = 0;
    $executable = 0;
    $covered = 0;
    foreach ($files as $file) {
      $row = array();
      $row[] = l($file->path, "code_coverage/$coverage_set/$file->file_id");
      $row[] = $file->executed;
      $row[] = $file->executable;
      $row[] = @($coverage = number_format(($file->executed / $file->executable) * 100)) . '%';

      $table['#rows'][] = array(
        'data' => $row,
        'class' => array('code-coverage-' . ($coverage > CODE_COVERAGE_COVERED ? 'covered' : 'uncovered')),
      );

      // Keep a running total of lines executed and executable.
      $executed += $file->executed;
      $executable += $file->executable;

      // Keep track of the number of files that have at least partial coverage.
      if ($file->executed > 0) {
        $covered++;
      }
    }

    return array(
      array(
        '#theme' => 'table',
        '#rows' => array(
          array(
            'data' => array(t('Total coverage'), @($coverage = number_format(($executed / $executable) * 100)) . '%'),
            'class' => array('code-coverage-' . ($coverage > CODE_COVERAGE_COVERED ? 'covered' : 'uncovered')),
          ),
          array(
            'data' => array(t('Total files covered'), @($coverage = number_format(($covered / count($files)) * 100)) . '%'),
            'class' => array('code-coverage-' . ($coverage > CODE_COVERAGE_COVERED ? 'covered' : 'uncovered')),
          ),
        ),
        '#attributes' => array('class' => array('code-coverage')),
        '#caption' => t('Summary'),
      ),
      $table,
    );
  }

  drupal_set_message(t('Invalid coverage set.'), 'error');
  return array();
}

/**
 * Get the list of files related to a coverage set.
 *
 * @param $coverage_set
 *   Coverage set ID.
 * @return
 *   List of of files.
 */
function code_coverage_report_file_list($coverage_set) {
  return db_select('code_coverage', 'c')
    ->fields('c', array('file_id', 'path', 'executed', 'executable'))
    ->condition('coverage_set', $coverage_set)
    ->orderBy('path')
    ->execute()
    ->fetchAll();
}

/**
 * View the detailed report for a file.
 *
 * @param $coverage_set
 *   Coverage set ID.
 * @param $file_id
 *   File ID.
 * @return
 *   Either a renderable array or nothing.
 */
function code_coverage_report_view_file($coverage_set, $file_id) {
  drupal_add_css(drupal_get_path('module', 'code_coverage') . '/code_coverage.css');

  // Load the coverage data for the file ID.
  $result = db_select('code_coverage', 'c')
    ->fields('c', array('path', 'data'))
    ->condition('file_id', $file_id)
    ->execute()
    ->fetchAssoc();

  drupal_set_title($result['path']);

  $lines = file($result['path'], FILE_IGNORE_NEW_LINES);
  $lines[] = ''; // The last line is ignored if it is blank.
  $coverage = json_decode($result['data'], TRUE);
  foreach ($lines as $number => &$line) {
    $line = htmlentities($line);

    if (isset($coverage[$number + 1])) {
      $line = array(
        'data' => $line,
        'class' => array('code-coverage-' . ($coverage[$number + 1] ? 'covered' : 'uncovered')),
      );
    }
  }

  return array(
    array(
      '#markup' => l(t('return to index'), 'code_coverage/' . $coverage_set),
    ),
    array(
      '#theme' => 'item_list',
      '#items' => $lines,
      '#type' => 'ol',
      '#attributes' => array('id' => 'code-coverage-file'),
    ),
  );
}
