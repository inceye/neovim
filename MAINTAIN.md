Maintaining the Neovim project
==============================

Notes on maintaining the Neovim project.

General guidelines
------------------

* Decide by cost-benefit
* Write down what was decided
* Constraints are good
* Use automation to solve problems
* Never break the API... but sometimes break the UI

Ticket triage
-------------

In practice we haven't found a way to forecast more precisely than "next" and
"after next". So there are usually one or two (at most) planned milestones:

- Next bugfix-release (1.0.x)
- Next feature-release (1.x.0)

The forecasting problem might be solved with an explicit priority system (like
Bram's todo.txt). Meanwhile the Neovim priority system is defined by:

- PRs nearing completion.
- Issue labels. E.g. the `+plan` label increases the ticket's priority merely
  for having a plan written down: it is _closer to completion_ than tickets
  without a plan.
- Comment activity or new information.

Anything that isn't in the next milestone, and doesn't have a finished PR—is
just not something you care very much about, by construction. Post-release you
can review open issues, but chances are your next milestone is already getting
full... :)

Release policy
--------------

Release "often", but not "early".

The (unreleased) `master` branch is the "early" channel; it should not be
released if it's not stable. High-risk changes may be merged to `master` if
the next release is not imminent.

For maintenance releases, create a `release-x.y` branch. If the current release
has a major bug:

1. Fix the bug on `master`.
2. Cherry-pick the fix to `release-x.y`.
3. Cut a release from `release-x.y`.
    - Run `./scripts/release.sh`
    - Update (force-push) the remote `stable` tag.
    - The [nightly job](https://github.com/neovim/bot-ci/blob/master/ci/nightly.sh)
      will update the release assets based on the `stable` tag.

See also
--------

- https://github.com/neovim/neovim/issues/862
- https://github.com/git/git/blob/master/Documentation/howto/maintain-git.txt
