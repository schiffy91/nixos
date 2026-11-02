# Agent configuration

`default.nix` applies the clients, rules and skills selected through
`settings.apps.*` on every host. `skills.nix` indexes the skills under
`skills/`; `settings.apps.agents.skills` picks which ones a host installs.
Home Manager handles file linking and conflict backups.

`settings.apps.agents.rules` installs `rules.md` for Claude, Codex and Gemini.
The Mac turns it on. Cloud instruction exports and an LM Studio preset are not
generated.

Humanizer's skill text and license remain unchanged from the revision recorded
in `upstream.json`. Its upstream CI and Python validator are excluded here.
The complete original copy remains in `~/Home`.
