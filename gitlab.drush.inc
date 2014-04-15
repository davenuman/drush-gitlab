<?php

/**
 * @file
 * This implements a drush command ui for the gitlab api.
 *
 * To use these commands you need to add two variables to your
 * .drush/drushrc.php for configuration. For example, add these lines:
 * $GLOBALS['gitlab_token'] = 'GiTdrUpal8+usEdrusH';
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
  $items['gitlab-add-deploy-key'] = array(
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'description' => dt('Add a deploy key to a project.'),
    'options' => array(
      'pid' => 'The project id.',
      'title' => 'A name for the key being added.',
      'filename' => 'The filename of the public key file to add.',
    ),
    'aliases' => array('gl-adk'),
  );
  $items['gitlab-add-user-key'] = array(
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'description' => dt('Add a key to a user.'),
    'arguments' => array(
      'id' => dt('The user id.'),
      'filename' => dt('The filename of the public key file to add.'),
    ),
    'options' => array(
      'title' => dt('A name for the key being added, defaults to the username'),
    ),
    'aliases' => array('gl-auk'),
  );
  $items['gitlab-create-user'] = array(
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'description' => dt('Create a new user.'),
    'arguments' => array(
      'email' => dt('Email Address.'),
      'username' => dt('Username.'),
      'password' => dt('Password.'),
    ),
    'aliases' => array('gl-cu'),
  );
  $items['gitlab-list-users'] = array(
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'description' => dt('List users.'),
    'aliases' => array('gl-lu'),
  );
  $items['gitlab-list-keys'] = array(
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'description' => dt('List ssh keys.'),
    'options' => array(
      'id' => dt('If sudo access, list keys for this user id.'),
      'format' => dt('Display format: table or authorized_keys'),
    ),
    'aliases' => array('gl-lk'),
  );
  $items['gitlab-api-get'] = array(
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'description' => dt('Use the api with provided GET path to get raw output.'),
    'arguments' => array(
      'path' => dt('The api path to use.'),
    ),
    'aliases' => array('gl-get'),
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

    case 'drush:gitlab-create-user':
      return dt('Create a new user.');

    case 'drush:gitlab-list-projects':
      return dt('List project to which you have access.');

    case 'drush:gitlab-list-users':
      return dt('List users.');

    case 'drush:gitlab-list-keys':
      return dt('List ssh keys.');
  }
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
  if (empty($GLOBALS['gitlab_' . $key])) {
    drush_die('$GLOBALS[\'gitlab_' . $key . '\'] not defined in .drushrc file.');
  }
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
    if (!empty($post_data['sudo'])) {
      $headers[] = 'SUDO: ' . $post_data['sudo'];
      unset($post_data['sudo']);
    }
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

/**
 * Get the id of the last project added.
 *
 * @return int
 *   The last project id.
 */
function _gitlab_get_last_pid() {
  $ids = array();
  foreach (_gitlab_api('projects') as $project) {
    $ids[] = $project['id'];
  }
  return max($ids);
}

/**
 * Drush command callback for creating projects.
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
 * Drush command callback for listing projects.
 */
function drush_gitlab_list_projects() {
  $rows = array(
    array('id', 'NAME', 'ssh url to repo', 'web url'),
    array('--', '----', '---------------', '-------'),
  );

  foreach (_gitlab_api('projects') as $project) {
    $rows[] = array(
      $project['id'],
      $project['name'],
      $project['ssh_url_to_repo'],
      $project['web_url'],
    );
  }

  drush_print_table($rows, TRUE);
}

/**
 * Drush command callback for adding deploy keys.
 */
function drush_gitlab_add_deploy_key() {
  $pid = drush_get_option('pid', _gitlab_get_last_pid());
  $title = drush_get_option('title', gethostname());
  $filename = drush_get_option('filename', getenv('HOME') . '/.ssh/id_rsa.pub');
  $post_data = array(
    'id' => $pid,
    'title' => $title,
    'key' => file_get_contents($filename),
  );

  $result = _gitlab_api('projects/' . $pid . '/keys', 'POST', $post_data);

  drush_print_r($result);
}

/**
 * Drush command callback for adding user keys.
 */
function drush_gitlab_add_user_key($id, $filename) {
  $user = _gitlab_api('users/' . $id);
  if (empty($user)) {
    drush_log(dt('User not found. Try drush gl-lu to get list of user ids.'), 'error');
    return FALSE;
  }
  $title = drush_get_option('title', $user['username']);

  $post_data = array(
    'id' => $id,
    'title' => $title,
    'key' => file_get_contents($filename),
  );
  print_r($post_data);

  $result = _gitlab_api('users/' . $id . '/keys', 'POST', $post_data);

  if (is_array($result)) {
    drush_log(dt('Key added.'));
    return TRUE;
  }
  else {
    drush_log(dt('Error adding key.'), 'error');
    return FALSE;
  }
}

/**
 * Drush command callback for creating users.
 */
function drush_gitlab_create_user($email, $username, $password, $name = FALSE) {
  if (strlen($password) < 8) {
    drush_log('Password must be at least 8 characters.', 'error');
    return FALSE;
  }
  $post_data = array(
    'email' => $email,
    'password' => $password,
    'username' => $username,
    'name' => (empty($name) ? $username : $name),
  );
  print_r($post_data);

  $result = _gitlab_api('users', 'POST', $post_data);
  // The result should be a json output of the user.
  drush_print_r($result);
}

/**
 * Drush command callback for listing users.
 */
function drush_gitlab_list_users() {
  $rows = array(
    array('id', 'NAME', 'Username', 'Email'),
    array('--', '----', '--------', '-----'),
  );

  foreach (_gitlab_api('users') as $user) {
    $rows[] = array(
      $user['id'],
      $user['name'],
      $user['username'],
      $user['email'],
    );
  }

  drush_print_table($rows, TRUE);
}

/**
 * Drush command callback for listing users.
 */
function drush_gitlab_list_keys() {
  $id = drush_get_option('id', FALSE);
  $key_function = drush_get_option('format', 'table') == 'table' ? '_gitlab_list_keys' : '_gitlab_print_keys';

  if (empty($id)) {
    $user = _gitlab_api('user');
    $user['current'] = TRUE;
    return $key_function(array($user));
  }
  elseif ($id == 'all') {
    $users = _gitlab_api('users');
    return $key_function($users);
  }
  else {
    $user = _gitlab_api('users/' . $id);
    if (empty($user)) {
      drush_log(dt('User not found. Try drush gl-lu to get list of user ids.'), 'error');
      return FALSE;
    }
    else {
      return $key_function(array($user));
    }
  }

}

/**
 * Helper function for listing user keys.
 */
function _gitlab_list_keys(array $users) {
  $rows = array(
    array('id', 'User', 'Title', 'Key'),
    array('--', '----', '-----', '---'),
  );

  foreach ($users as $user) {
    $keys = !empty($user['current']) ? _gitlab_api('user/keys') :
      _gitlab_api('user/keys', 'GET', array('sudo' => $user['id']));
    foreach ($keys as $key) {
      $key_parts = explode(' ', $key['key']);
      $key_parts[1] = substr($key_parts[1], 0, 8) . '...' . substr($key_parts[1], -8);
      $trimkey = implode(' ', $key_parts);
      $rows[] = array(
        $key['id'],
        $user['username'],
        $key['title'],
        $trimkey,
      );
    }
  }

  drush_print_table($rows, TRUE);
  return TRUE;
}

/**
 * Print ssh keys formatted for an authorized_keys file.
 */
function _gitlab_print_keys(array $users) {
  foreach ($users as $user) {
    $keys = !empty($user['current']) ? _gitlab_api('user/keys') :
      _gitlab_api('user/keys', 'GET', array('sudo' => $user['id']));
    if (count($keys)) {
      drush_print("\n## " . $user['username']);
    }
    foreach ($keys as $key) {
      drush_print($key['key']);
    }
  }
  return TRUE;
}

/**
 * Use the GET api with provided path.
 */
function drush_gitlab_api_get($path) {
  $result = _gitlab_api($path);
  drush_print_r($result);
}
