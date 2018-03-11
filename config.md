# configuration

The bot reads all config options from environment variables.

```
BOT_NICK - Nick that the bot uses
BOT_USER - The ident for the bot
BOT_NAME - The realname of the bot
BOT_NETWORK_HOST - The host irc server
BOT_NETWORK_PORT - The port of the irc server
BOT_NETWORK_SSL - Whether or not to use SSL (default to '0', set to '1' to enable SSL)
BOT_NETWORK_PASSWORD - A password for the server, if applicable (use 'nickserv username:nickserv pass' for SASL)
BOT_PREFIX - The prefix to use (if omitted, defaults to ',')
BOT_MODULES_DIR - The directory where modules are (if omitted, defaults to modules/)
BOT_OWNER_HOST - The host of the owner (if omitted, there is no owner)
```
