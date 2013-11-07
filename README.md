SUMMARY
-------

This is a fork of the Drupal contributed Code Coverage module, with a few
changes and improvements (https://drupal.org/project/code_coverage). It uses
the PHP xdebug framework to determine coverage for tests that are run.

To use, simply run tests using the Drupal simpletest module. If any files are
covered, a link will be added to the results page to check the statistics and
view individual files.

REQUIREMENTS
------------

The php5-xdebug extension must be installed and configured, and then the
Apache service must be restarted. Configuring xdebug involves adding the path
to libxdebug.so to php.ini, like so:
> zend_extension="path/to/xdebug.so"

This fork has not been sanitized against Windows paths and WILL NOT WORK on a
Windows Drupal installation. And I ain't got time to make it work, so blech.

INSTALLATION
------------

Before the module can be installed, the patch included in the module directory
must be applied to Drupal core. This can be done by copying the patch to your
DRUPAL_ROOT directory and running the command:
> patch -p1 < code_coverage.patch

CONFIGURATION
-------------

Filters for whitelisted files and modules can be set in the code coverage admin
form at admin/config/development/code_coverage.

DRUSH COMMANDS
--------------

The biggest change to this fork of code_coverage is to the ways Drush can work.
More options have been added, and the schema has been updated to ensure the
module can work completely form the command line in an automated context, such
as with a continuous integration server, without having to make note of the IDs
of test runs.

Drush supports the following commands:
 - code-coverage-process (cc-process) - processes a coverage set of raw Xdebug
data. You can run it as "cc-process set_id" to process an individual set, or
run "cc-process all" to process all sets that are currently unprocessed.
 - code-coverage-export (cc-export) - exports a code coverage set. It supports
HTML, XML, JSON and CSV formats using the tags --html, --xml, --json and --csv,
respectively. HTML can be further configured using the --tag-html and
--generate-index tags, which, respectively, enclose the HTML files in <html>
and <body> tags, and generate an index file. Lastly, the --path tag can be used
to change the path to the exported files from the default /tmp/code_coverage_id
folder.
 - code-coverage-executable (cc-executable) - Lists the executable lines in a
file designated by the --path tag.
 - code-coverage-list-unprocessed (cc-list) - Dumps a list of unprocessed set
IDs to stdout.
