require 'test_helper'

class ChatStatementTest < ActiveSupport::TestCase

  test 'Create a simple chat statement' do
    statement = ChatStatement.new(
      speaker: ChatStatement.speakers[:bot],
      nature: ChatStatement.natures[:text],
      content: { text: 'Good morning !' },
      chat_session: chat_sessions(:one)
    )
    assert statement.save
  end


  test 'Validate chat statement' do
    statement = ChatStatement.new(
      speaker: ChatStatement.speakers[:bot],
      nature: ChatStatement.natures[:text],
      content: '',
      chat_session: chat_sessions(:one)
    )
    assert statement.invalid?
    expected = [
      "Content can't be blank",
    ]
    assert_equal expected, statement.errors.full_messages
  end


  test 'Validate text statement' do
    statement = ChatStatement.new(
      speaker: ChatStatement.speakers[:bot],
      nature: ChatStatement.natures[:text],
      content: { text: '' },
      chat_session: chat_sessions(:one)
    )
    assert statement.invalid?
    expected = [
      "Text can't be blank"
    ]
    assert_equal expected, statement.errors.full_messages

    statement.content = { text: 'a' * 5001 }
    assert statement.invalid?
    assert_equal ['Text is too long (maximum is 5000 characters)'], statement.errors.full_messages
  end


  test 'Validate text statement with speech' do
    statement = ChatStatement.new(
      speaker: ChatStatement.speakers[:bot],
      nature: ChatStatement.natures[:text],
      content: {
        text: 'Hello',
        speech: ''
      },
      chat_session: chat_sessions(:one)
    )
    assert statement.invalid?
    expected = ["Speech must be a Hash"]
    assert_equal expected, statement.errors.full_messages

    statement.content = {
      text: 'Hello',
      speech: {
        text: '',
        locale: ''
      }
    }
    assert statement.invalid?
    expected = ["Text can't be blank, Locale is not included in the list"]
    assert_equal expected, statement.errors.full_messages

    statement.content = {
      text: 'Hello',
      speech: {
        text: 'Hi',
        locale: 'en-US'
      }
    }
    assert statement.valid?
  end


  test 'Create a simple list statement' do
    statement_list = ChatStatement.new(
      speaker: ChatStatement.speakers[:bot],
      nature: ChatStatement.natures[:list],
      content: {
        list: [
          {
            nature: 'text',
            text: 'Hello'
          },
          {
            nature: 'text',
            text: 'How are you ?'
          }
        ]
      },
      chat_session: chat_sessions(:one)
    )
    assert statement_list.save
  end


  test 'Validate a list statement' do
    statement_list = ChatStatement.new(
      speaker: ChatStatement.speakers[:bot],
      nature: ChatStatement.natures[:list],
      content: {
        list: [
          {
            nature: 'text',
            text: 'Hello'
          },
          {
            nature: 'text',
            text: 'How are you ?'
          },
          {
            nature: 'text',
            text: 'What are you doing ?'
          },
          {
            nature: 'text',
            text: 'What is your name ?'
          },
          {
            nature: 'text',
            text: 'TALK TO ME !!!'
          }
        ]
      },
      chat_session: chat_sessions(:one)
    )
    assert statement_list.invalid?
    assert_equal ['List is too long (maximum is 4 items)'], statement_list.errors.full_messages

    statement_list.content = { list: [] }
    assert statement_list.invalid?
    assert_equal ["List can't be blank"], statement_list.errors.full_messages

    statement_list.content = {
      list: [{
        list: []
      }]
    }
    assert statement_list.invalid?
    assert_equal ['invalid nature'], statement_list.errors.full_messages

    statement_list.content = {
      list: [
        {
          nature: 'text',
          text: ''
        }
      ]
    }
    assert statement_list.invalid?
    assert_equal ["Text can't be blank"], statement_list.errors.full_messages
  end


  test 'Create a simple image statement' do
    statement_image = ChatStatement.new(
      speaker: ChatStatement.speakers[:bot],
      nature: ChatStatement.natures[:image],
      content: {
        url: 'https://www.pertimm.com/assets/img/logo_pertimm.png',
        title: 'Pertimm',
        subtitle: 'The Pertimm logo.'
      },
      chat_session: chat_sessions(:one)
    )
    assert statement_image.save
  end


  test 'Validate image statement' do
    statement_image = ChatStatement.new(
      speaker: ChatStatement.speakers[:bot],
      nature: ChatStatement.natures[:image],
      content: {
        url: '',
      },
      chat_session: chat_sessions(:one)
    )

    assert statement_image.invalid?
    assert_equal ["Url can't be blank"], statement_image.errors.full_messages

    statement_image.content['url'] = 'a' * 5001
    statement_image.content['title'] = 'a' * 5001
    statement_image.content['subtitle'] = 'a' * 5001
    assert statement_image.invalid?
    expected = [
      'Url is too long (maximum is 5000 characters)',
      'Title is too long (maximum is 5000 characters)',
      'Subtitle is too long (maximum is 5000 characters)',
    ].join(', ')
    assert_equal [expected], statement_image.errors.full_messages
  end


  test 'Create a simple button statement' do
    statement_button = ChatStatement.new(
      speaker: ChatStatement.speakers[:bot],
      nature: ChatStatement.natures[:button],
      content: {
        text: 'Self destruction',
        payload: {
          destruction: true,
          timer: '5 seconds'
        }
      },
      chat_session: chat_sessions(:one)
    )
    assert statement_button.save
  end


  test 'Validate button statement' do
    statement_button = ChatStatement.new(
      speaker: ChatStatement.speakers[:bot],
      nature: ChatStatement.natures[:button],
      content: {
        text: '',
        payload: {}
      },
      chat_session: chat_sessions(:one)
    )
    assert statement_button.invalid?
    assert_equal ["Text can't be blank, Payload can't be blank"], statement_button.errors.full_messages

    statement_button.content['text'] = 'a' * 2001
    statement_button.content['payload'] = { a: 'a' }
    assert statement_button.invalid?
    assert_equal ['Text is too long (maximum is 2000 characters)'], statement_button.errors.full_messages
  end


  test 'Create a simple notification statement' do
    statement_notification = ChatStatement.new(
      speaker: ChatStatement.speakers[:moderator],
      nature: ChatStatement.natures[:notification],
      content: {
        text: 'Bot error',
      },
      chat_session: chat_sessions(:one)
    )
    assert statement_notification.save
  end


  test 'Validate notification statement' do
    statement_notification = ChatStatement.new(
      speaker: ChatStatement.speakers[:moderator],
      nature: ChatStatement.natures[:notification],
      content: {
        text: ''
      },
      chat_session: chat_sessions(:one)
    )
    assert statement_notification.invalid?
    assert_equal ["Text can't be blank"], statement_notification.errors.full_messages

    statement_notification.content['text'] = 'a' * 2001
    assert statement_notification.invalid?
    assert_equal ['Text is too long (maximum is 2000 characters)'], statement_notification.errors.full_messages
  end
end