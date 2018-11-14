# minervabot

It's a modular IRC bot written in C!

## about

minverabot is a simple multi-threaded bot written in C.

It is the continuation of [an old effort to write an IRC bot](https://github.com/ohnx-archive/athena).

## features

  * modular
  * reload modules without restarting the bot
  * commands run in threads (all bot interface functions are threadsafe)
  * SSL support

## configuration

The bot reads all config options from environment variables.

```
BOT_NICK - Nick that the bot uses
BOT_USER - The ident for the bot
BOT_NAME - The realname of the bot
BOT_LOG_VERBOSITY - The verbosity of the bot on stdout (default to 1, 2 = raw send/receive, 1 = messages only, 0 = nothing)
BOT_NETWORK_HOST - The host irc server
BOT_NETWORK_PORT - The port of the irc server
BOT_NETWORK_SSL - Whether or not to use SSL (default to '0', set to '1' to enable SSL)
BOT_NETWORK_PASSWORD - A password for the server, if applicable (use 'nickserv username:nickserv pass' for SASL)
BOT_PREFIX - The prefix to use (if omitted, defaults to ',')
BOT_MODULES_DIR - The directory where modules are (if omitted, defaults to modules/)
BOT_OWNER_HOST - The host of the owner (if omitted, there is no owner)
```

## how to reload modules

To reload modules, send a SIGUSR1 to the bot's PID. e.g.

```
kill -USR1 <bot pid>
```

The bot outputs its PID when it launches.

## developing modules

Please see some of the included modules in `src/modules/` and the primary module API in `include/module.h`.
