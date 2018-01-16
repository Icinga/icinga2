#!/usr/bin/env python
# -*- coding:utf-8 -*-

import requests
import re
import pickle
import sys
import os
from datetime import datetime
from collections import defaultdict
from collections import OrderedDict

#################################
## Env Config

try:
    github_auth_username = os.environ['ICINGA_GITHUB_AUTH_USERNAME']
except KeyError:
    print "ERROR: Environment variable 'ICINGA_GITHUB_AUTH_USERNAME' is not set."
    sys.exit(1)

try:
    github_auth_token = os.environ['ICINGA_GITHUB_AUTH_TOKEN']
except:
    print "ERROR: Environment variable 'ICINGA_GITHUB_AUTH_TOKEN' is not set."
    sys.exit(1)

try:
    project_name = os.environ['ICINGA_GITHUB_PROJECT']
except:
    print "ERROR: Environment variable 'ICINGA_GITHUB_PROJECT' is not set."
    sys.exit(1)

#################################
## Config

changelog_file = "CHANGELOG.md" # TODO: config param
debug = 1

# Keep this in sync with GitHub labels.
ignored_labels = [
    "high-priority", "low-priority",
    "bug", "enhancement",
    "needs-feedback", "question", "duplicate", "invalid", "wontfix",
    "backported", "build-fix"
]

# Selectively show and collect specific categories
#
# (category, list of case sensitive matching labels)
# The order is important!
# Keep this in sync with GitHub labels.
categories = OrderedDict(
[
    ("Enhancement", ["enhancement"]),
    ("Bug", ["bug", "crash"]),
    ("ITL", ["ITL"]),
    ("Documentation", ["Documentation"]),
    ("Support", ["code-quality", "Tests", "Packages", "Installation"])
]
)

#################################
## Helpers

def write_changelog(line):
    clfp.write(line + "\n")

def log(level, msg):
    if level <= debug:
        print " " + msg

def fetch_github_resources(uri, params = {}):
    resources = []

    url = 'https://api.github.com/repos/' + project_name + uri + "?per_page=100" # 100 is the maximum

    while True:
        log(2, "Requesting URL: " + url)
        resp = requests.get(url, auth=(github_auth_username, github_auth_token), params=params)
        try:
            resp.raise_for_status()
        except requests.exceptions.HTTPError as e:
            raise e

        data = resp.json()

        if len(data) == 0:
            break

        resources.extend(data)

        # fetch the next page from headers, do not count pages
        # http://engineering.hackerearth.com/2014/08/21/python-requests-module/
        if "next" in resp.links:
            url = resp.links['next']['url']
            log(2, "Found next link for Github pagination: " + url)
        else:
            break # no link found, we are done
            log(2, "No more pages to fetch, stop.")

    return resources

def issue_type(issue):
    issue_labels = [label["name"] for label in issue["labels"]]

    # start with the least important first (e.g. "Support", "Documentation", "Bug", "Enhancement" as order)
    for category in reversed(categories):
        labels = categories[category]

        for label in labels:
            if label in issue_labels:
                return category

    return "Support"

def escape_markdown(text):
    #tmp = text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
    tmp = text
    tmp.replace('\\', '\\\\')

    return re.sub("([<>*_()\[\]#])", r"\\\1", tmp)

def format_labels(issue):
    labels = filter(lambda label: label not in ignored_labels, [label["name"] for label in issue["labels"]])

    # Mark PRs as custom label
    if "pull_request" in issue:
        labels.append("PR")

    if len(labels):
        return " (" + ", ".join(labels) + ")"
    else:
        return ""

def format_title(title):
    # Fix encoding
    try:
        issue_title = str(title.encode('ascii', 'ignore').encode('utf-8'))
    except Error:
        log(1, "Error: Cannot convert " + title + " to UTF-8")

    # Remove dev.icinga.com tag
    issue_title = re.sub('\[dev\.icinga\.com #\d+\] ', '', issue_title)

    #log(1, "Issue title: " + issue_title + "Type: " + str(type(issue_title)))

    return escape_markdown(issue_title)

#################################
## MAIN

milestones = {}
issues = defaultdict(lambda: defaultdict(list))

log(1, "Fetching data from GitHub API for project " + project_name)

try:
    tickets = fetch_github_resources("/issues", { "state": "all" })
except requests.exceptions.HTTPError as e:
    log(1, "ERROR " + str(e.response.status_code) + ": " + e.response.text)

    sys.exit(1)

clfp = open(changelog_file, "w+")

with open('tickets.pickle', 'wb') as fp:
    pickle.dump(tickets, fp)

with open('tickets.pickle', 'rb') as fp:
    cached_issues = pickle.load(fp)

for issue in cached_issues: #fetch_github_resources("/issues", { "state": "all" }):
    milestone = issue["milestone"]

    if not milestone:
        continue

    ms_title = milestone["title"]

    if not re.match('^\d+\.\d+\.\d+$', ms_title):
        continue

    if ms_title.split(".")[0] != "2":
        continue

    milestones[ms_title] = milestone

    ms_tickets = issues[ms_title][issue_type(issue)]
    ms_tickets.append(issue)

# TODO: Generic header based on project_name
write_changelog("# Icinga 2.x CHANGELOG")
write_changelog("")

for milestone in sorted(milestones.values(), key=lambda ms: (ms["due_on"], ms["title"]), reverse=True):
    if milestone["state"] != "closed":
        continue

    if milestone["due_on"] == None:
        print "Milestone", milestone["title"], "does not have a due date."
        sys.exit(1)

    ms_due_on = datetime.strptime(milestone["due_on"], "%Y-%m-%dT%H:%M:%SZ")

    write_changelog("## %s (%s)" % (milestone["title"], ms_due_on.strftime("%Y-%m-%d")))
    write_changelog("")

    ms_description = milestone["description"]
    ms_description = re.sub('\r\n', '\n', ms_description)

    if len(ms_description) > 0:
        write_changelog("### Notes\n\n" + ms_description + "\n") # Don't escape anything, we take care on Github for valid Markdown

    for category, labels in categories.iteritems():
        try:
            ms_issues = issues[milestone["title"]][category]
        except KeyError:
            continue

        if len(ms_issues) == 0:
            continue

        write_changelog("### " + category)
        write_changelog("")

        for issue in ms_issues:
            write_changelog("* [#" + str(issue["number"]) + "](https://github.com/" + project_name
                + "/issues/" + str(issue["number"]) + ")" + format_labels(issue) + ": " + format_title(issue["title"]))

        write_changelog("")

clfp.close()
log(1, "Finished writing " + changelog_file)
