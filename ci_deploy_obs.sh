#!/usr/bin/env bash

################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  READ THE ZPROJECT/README.MD FOR INFORMATION ABOUT MAKING PERMANENT CHANGES. #
################################################################################

# do NOT set -x or it will log the secret tokens!
set -e

if [ "$BUILD_TYPE" == "default" -a -n "${GH_TOKEN}" -a -n "${OBS_STABLE_TOKEN}" -a -n "${OBS_DRAFT_TOKEN}" ]; then
    # Trigger source run on new tag on OBS. See travis.yml for token instructions.
    # We have to create a temporary branch from the tag and delete it, as it is
    # not possible to edit files on OBS with secure tokens, and it is not
    # possible to dynamically fetch the latest git tag either.
    TAG_SHA=$(curl -s -H "Authorization: token ${GH_TOKEN}" -X GET https://api.github.com/repos/42ity/fty-metric-snmp/git/refs/tags/${TRAVIS_TAG} | grep -o -P '(?<=sha":\s).*(?=,)')
    curl -H "Authorization: token ${GH_TOKEN}" -X POST --data "{\"ref\":\"refs/heads/tmp_obs_release_branch\",\"sha\":${TAG_SHA}}" https://api.github.com/repos/42ity/fty-metric-snmp/git/refs
    curl -H "Authorization: Token ${OBS_STABLE_TOKEN}" -X POST https://api.opensuse.org/trigger/runservice
    curl -H "Authorization: Token ${OBS_DRAFT_TOKEN}" -X POST https://api.opensuse.org/trigger/runservice
    # give some time for the git clone to happen before deleting the temp branch
    sleep 60
    curl -H "Authorization: token ${GH_TOKEN}" -X DELETE https://api.github.com/repos/42ity/fty-metric-snmp/git/refs/heads/tmp_obs_release_branch
fi