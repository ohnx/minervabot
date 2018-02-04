# configuration

The bot reads all config options from environment variables.

```
BOT_NICK - Nick that the bot uses
BOT_USER - The ident for the bot
BOT_NAME - The realname of the bot
BOT_NETWORK_HOST - The host irc server
BOT_NETWORK_PORT - The port of the irc server
BOT_NICKSERV - The nickserv command to run on connect (if omitted, nothing will be run)
    e.g. PRIVMSG NickServ :identify bot botpassword
BOT_PREFIX - The prefix to use (if omitted, defaults to ',')
BOT_MODULES_DIR - The directory where modules are (if omitted, defaults to modules/)
BOT_OWNER_HOST - The host of the owner (if omitted, there is no owner)
```
