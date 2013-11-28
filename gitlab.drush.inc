<?php

/**
 * @file
 * This implements a drush command ui for the gitlab api.
 *
 * To use these commands you need to add two variables to your .drushrc.php for
 * configuration. For example, add these lines:
 * $GLOBALS['gitlab_token'] = 'xxx';
 * $GLOBALS['gitlab_url'] = 'http://example.com';
 *
 * Get your api token from your gitlab site at /profile/account
 */

/**
 * Implements hook_drush_command().
 */
function gitlab_drush_command() {
  $items = array();

  $items['gitlab-create-project'] = array(
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'description' => dt('Create a new project.'),
    'arguments' => array(
      'name' => dt('The Project Name.'),
    ),
    'aliases' => array('gl-cp'),
  );
  $items['gitlab-list-projects'] = array(
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'description' => dt('List projects.'),
    'aliases' => array('gl-lp'),
  );

  return $items;
}

/**
 * Implements hook_drush_help().
 */
function gitlab_drush_help($section) {
  switch ($section) {
    case 'drush:gitlab-create-project':
      return dt('Create a new project.');

    case 'drush:gitlab-list-projects':
      return dt('List project to which you have access.');
  }
}

/**
 * Drush command callback.
 */
function drush_gitlab_create_project($project_name = '') {
  $post_data = array(
    'name' => $project_name,
    // You can also edit these in the project settings page later.
    'description' => '',
    'issues_enabled' => FALSE,
    'wall_enabled' => FALSE,
    'merge_requests_enabled' => FALSE,
    'wiki_enabled' => FALSE,
    'snippets_enabled' => FALSE,
    'public' => FALSE,
  );

  $result = _gitlab_api('projects', 'POST', $post_data);
  // The result should be a json output of the project.
  drush_print_r($result);
}

/**
 * Drush command callback.
 */
function drush_gitlab_list_projects() {
  $rows = array(
    array('NAME', 'ssh_url_to_repo', 'web_url'),
    array('----', '---------------', '-------'),
  );

  foreach (_gitlab_api('projects') as $project) {
    $rows[] = array(
      $project['name'],
      $project['ssh_url_to_repo'],
      $project['web_url'],
    );
  }

  drush_print_table($rows, TRUE);
}

/**
 * Simple function that gets config from a global variable.
 *
 * The intention is to create a more robust config later with yaml or simply a
 * ~/.gitlab.conf file. For now, place the variables in ~/.drushrc
 *
 * @return string
 *   The requested configuration value.
 */
function _gitlab_get_config($key) {
  return $GLOBALS['gitlab_' . $key];
}

/**
 * The gitlab api layer.
 *
 * @param string $path
 *   The api path.
 * @param string $method
 *   Specify the http method.
 * @param array $post_data
 *   Information to post to the api.
 *
 * @return array
 *   A json array of information depending on the request.
 */
function _gitlab_api($path, $method = 'GET', $post_data = NULL) {
  $headers = array('PRIVATE-TOKEN: ' . _gitlab_get_config('token'));
  $http = array('method' => $method);

  if (!empty($post_data)) {
    $json_post = drush_json_encode($post_data);
    $headers[] = 'Content-type: application/json';
    $headers[] = 'Content-length: ' . strlen($json_post);
    $http['content'] = $json_post;
  }
  $http['header'] = implode("\r\n", $headers) . "\r\n";

  $url = sprintf('%s/api/v3/%s', _gitlab_get_config('url'), $path);
  $result = file_get_contents($url, NULL,
    stream_context_create(array('http' => $http))
  );

  return drush_json_decode($result);
}
