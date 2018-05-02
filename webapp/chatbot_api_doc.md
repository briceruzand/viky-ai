
# Chatbot

## Overview

### Initiating a session

The viky.ai chatbot starts a session and sends an id token to the bot.

```
                      (send session id)
viky.ai (chatbot) -----------------------> bot
```

### Typical chat interactions

The user sends a statement to the bot through the chatbot interface.
The bot receives the request and process it, usually resulting in an answer or at least saying that the request has been fulfilled.

```
                   (user action)
viky.ai (chatbot) -----------------> bot
   |                                  |    (interpret user action)
   |                                  | -----------------------------> viky.ai (interpret)
   |                                  |                                   |
   |                                  | <-------------------------------- |
   |                                  |            (an intent)
   |                                  |
   |                                  |   (request another service)
   |                                  | -----------------------------> external service
   |                                  |                                   |
   |                                  | <-------------------------------- |
   |                                  |            (answer)
   |                                  |
   | <------------------------------- |
                   (bot answer)
```

## API conventions

The API is based on the HTTP protocol.

The settings must be encoded in UTF-8 and respect the URL encoding.

The return in UTF-8 uses the JSON format.

Using APIs requires the Header HTTP `Accept: application/json` and the Header HTTP `Content-Type: application/json` to be systematically sent when the method used requires the sending of JSONs in the HTTP body.

## viky.ai API

Through its public API, viky.ai exposes an endpoint to listen for bot answers.
Before any conversation can start, make sure the bot URL is configured in the bot on viky.ai.

### Create statement

#### Endpoint

`POST /api/v1/chat_sessions/<session_id>/statements`

#### JSON
```
 {
   statement: {
     nature: <nature>,
     content: {
       <...>
     }
   }
 }
```

The nature is the type of widget which will be displayed to the user. The content object changes accordingly.

Statement natures currently available and their respective content.

#### <code>text</code> nature

Display text, URLs are automatically converted to HTML links.

JSON structure :
```
{
  statement: {
    nature: 'text',
    content: {
      text: <text>,
      speech: {
        text: <speech_text>,
        locale: <speech_locale>
      }
    }
  }
}
```

  * `<text>` Text to display (**required**).
  * `<speech_text>` Text to speech (via text to speech).
  * `<speech_locale>` Locale of the text to speech.

**Note:** Available speech locales are `ru-RU`, `ar`, `ja-JP`, `ko-KR`, `zh`, `en-US`, `en-GB`, `fr-FR`, `es-ES`, `it-IT` and `de-DE`.

#### <code>image</code> nature

Display an image with optional title and subtitle.

JSON structure :
```
{
  statement: {
    nature: 'image',
    content: {
      url: <url>,
      title: <title>,
      subtitle: <subtitle>,
      speech: {
        text: <speech_text>,
        locale: <speech_locale>
      }
    }
  }
}
```

  * `<url>` the image URL (**required**).
  * `<title>` a noteworthy title.
  * `<subtitle>` a short description.
  * `<speech_text>` Text to speech (via text to speech).
  * `<speech_locale>` Locale of the text to speech.

**Note:** Available speech locales are `ru-RU`, `ar`, `ja-JP`, `ko-KR`, `zh`, `en-US`, `en-GB`, `fr-FR`, `es-ES`, `it-IT` and `de-DE`.


#### <code>button</code> nature

JSON structure :
```
{
  statement: {
    nature: 'button',
    content: {
      text: <text>,
      payload: {
          <...>
      },
      speech: {
        text: <speech_text>,
        locale: <speech_locale>
      }
    }
  }
}
```

  * `text` is the displayed in the button (**required**).
  * `payload` must be a JSON which can contain anything. It will be returned as is to the bot when the user click on the button (**required**).
  * `<speech_text>` Text to speech (via text to speech).
  * `<speech_locale>` Locale of the text to speech.

**Note:** Available speech locales are `ru-RU`, `ar`, `ja-JP`, `ko-KR`, `zh`, `en-US`, `en-GB`, `fr-FR`, `es-ES`, `it-IT` and `de-DE`.


## Bot API

A bot is a remote service implementing the desired business logic.
It is reachable through a REST API and it **must include those following endpoints** in order to work with the viky.ai chatbot :

### Bot is alive

#### Endpoint

`GET /ping`

viky.ai wants to know if the bot is up and running mainly to display a status indicator for the user.

It expects an HTTP code `200 OK` if the bot is available. Any other code (or the lack of answer) will result in considering the bot is unavailable.

### Listen for new chat session

#### Endpoint

`POST /start`

#### JSON
```
{
  session_id: <id>
}
```
When a user starts a conversion from viky.ai, the bot will receive a new session id.

The bot must keep this session id if it wants to follow the context of several conversations at the same time.
For a specific bot, **only the last session is valid** and any request to a closed one will result in a `403 Forbidden` response.

### Listen for a new user action

#### Endpoint

`POST /sessions/<session_id>/user_actions`

#### JSON

```
{
  user_actions: {
    type: <type>,
    ...
  }
}
```

A user made a new statement in the conversation. It must include the current session id as URL parameter.


### Types

User actions types triggered by the user.

#### Type <code>says</code>

```
{
  user_actions: {
    type: 'says',
    text: <text>
  }
}
```

  * `text` the string typed by the user

#### Type <code>click</code>

```
{
  user_actions: {
    type: 'click',
    payload: {
      <...>
    }
  }
}
```

  * `payload` the JSON hash previously passed at the button creation