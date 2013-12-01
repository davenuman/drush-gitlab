Drush + Gitlab
==============

This is proof-of-concept code trying out the [gitlab api](http://api.gitlab.org/) for possible usage from a web server.

Currently the drush commands can:

1. List projects.
1. Create a new project.
1. Add a deploy key.

## Installation

1. Clone this repo.
2. Copy or link the gitlab.drush.inc file into your .drush directory.
3. Add token (from {sitename}/profile/account) and url variables to your .drushrc file.

