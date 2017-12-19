<?php
define('SFSIPS_BASE', dirname(__FILE__) . '/lib/base/');
define("SFSIPS_MODEL", dirname(__FILE__) . "/lib/models/");
define("SFSIPS_CTRL", dirname(__FILE__) . "/lib/controls/");
define("SFSIPS_VIEW", dirname(__FILE__) . "/lib/views/");

include_once "config.php";
include_once SFSIPS_BASE . "/db.php";
include_once SFSIPS_BASE . "/functions.php";
include_once SFSIPS_BASE . "/template.php";

include_once SFSIPS_VIEW . "/online_list.php";

$template = new Template;

$template ->set_filenames(array(
    'body' => 'html/body.html',
    'online_list' => 'html/online_list.html',
));

$template ->assign_var_from_handle("page_body", "online_list");
$template ->pparse('body');
?>