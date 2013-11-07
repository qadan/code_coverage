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

@TODO: this lol
