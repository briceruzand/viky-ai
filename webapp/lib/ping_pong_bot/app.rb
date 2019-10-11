require 'sinatra/base'
require 'sinatra/reloader'
require 'sinatra/json'
require 'rest-client'
require 'json'

$: << File.expand_path('../', __FILE__)
require 'lib/bot_api.rb'
require 'lib/bot_ressources.rb'

class PingPongBot < Sinatra::Base
  set :root, File.dirname(__FILE__)
  set :port, 3001

  # set :environment, :production

  configure :development do
    register Sinatra::Reloader
  end

  post '/start' do
    parameters = JSON.parse(request.body.read)
    session_id = parameters["session_id"]

    BotApi
      .list(
        BotRessources.intro.map { |text| BotApi::Params::build_text(text)},
        :horizontal,
      )
      .add_speech('Welcome to Ping Pong Bot', 'en-US')
      .send(session_id)

    status 200
    json Hash.new
  end


  post '/sessions/:session_id/user_actions' do
    sleep(0.5)

    session_id = params["session_id"]
    parameters = JSON.parse(request.body.read)

    case parameters['user_action']['type']

    when 'click'
      payload = parameters['user_action']['payload']

      case payload['action']
      when 'display_kitten'
        BotApi.image(BotRessources.kittens.sample).send(session_id)
      when 'display_puppy'
        BotApi.image(BotRessources.puppies.sample).send(session_id)
      when 'display_duckling'
        BotApi.image(BotRessources.ducklings.sample).send(session_id)
      when 'update_locale'
        BotApi.update_locale(session_id, payload['locale'])
      else
        nice_payload = JSON.pretty_generate(payload)
        BotApi.text("<p>You triggered with payload:</p><pre>#{nice_payload}</pre>").send(session_id)
      end

      when 'locate'
        if parameters['user_action']['status'] == 'success'
          location = parameters['user_action']['location']
          params = {
            api_key: "***REMOVED***", endpoint: "javascript", payload: {
              map: {
                center: {lat: location['coords']['latitude'], lng: location['coords']['longitude']},
                zoom: 12
              },
              markers: {
                list: [
                  { position: {lat: location['coords']['latitude'], lng: location['coords']['longitude']}, title: "You are here." },
                ]
              }
            }
          }
          BotApi.map(params, 'Found you !', '').send(session_id)
        else
          if parameters['user_action']['error']['code'] == 1
            BotApi.text("Ok I need a little help here... Please share your location.").send(session_id)
          else
            BotApi.text("Hmm... That's embarrassing, it seems that your are not able to send your location.").send(session_id)
          end
        end

    when "says"
      user_statement_says = parameters['user_action']['text']

      case user_statement_says

      when /help/i
        BotApi
          .list(
            BotRessources.intro(true).map { |text| BotApi::Params::build_text(text)},
            :horizontal,
          )
          .send(session_id)

      when /ping/i
        BotApi
          .text('Pong')
          .add_speech('Pong succeed', 'en-US')
          .send(session_id)

      when /pong/i
        BotApi
          .text('Ping')
          .add_speech('Ping succeed', 'en-US')
          .send(session_id)

      when /image/i
        BotApi
          .image(
            BotRessources.kittens.sample,
            'Here we love kittens',
            'The kittens are too cute. Do you agree?'
          )
          .add_speech('Voici une image de chatton', 'fr-FR')
          .send(session_id)

        when /map(-|\s)?(place|directions|search|view|streetview|markers)/i
          api_key = "***REMOVED***"
          params, title, description = case $2
            when 'place'
              [{api_key: api_key, endpoint: "place", query: "q=Valence"},
              "Valence (Drôme)",
              "Valence est une commune du sud-est de la France. Avec 62 150 habitants, elle est la ville la plus peuplée de la Drôme. Ses habitants sont appelés les Valentinois."]
            when 'directions'
              [{api_key: api_key, endpoint: "directions", query: "origin=Paris+France&destination=Valence+France"},
              "Itinéraire de Paris à Valence", '']
            when 'search'
              [{api_key: api_key, endpoint: "search", query: "q=Restaurant+Valence"},
              "Restaurants (Valence)", '']
            when 'view'
              [{api_key: api_key, endpoint: "view", query: "center=48.858281,2.294667&zoom=18&maptype=satellite"},
              "Tour Eiffel",
              "Célèbre tour en fer de Gustave Eiffel (1889), terrasses panoramiques accessibles par escaliers et ascenseurs."]
            when 'streetview'
              [{api_key: api_key, endpoint: "streetview", query: "location=44.929228,4.8887884&heading=-60&pitch=10"},
              "Kiosque Peynet (Valence)", '']
            when 'markers'
              [{api_key: api_key, endpoint: "javascript", payload: {
                map: {
                  center: {lat: 44.933146, lng: 4.892174},
                  zoom: 10
                },
                markers: {
                  center: true,
                  list: [
                    { position: { lat: 44.936115, lng: 4.924396 }, title: "Châteaux d’eau, parc Jean Perdrix", description: "La Sculpture-château d'eau de Philolaos" },
                    { position: { lat: 44.929882, lng: 4.886058 }, title: 'Parc Jouvet', description: "<p>Véritable poumon vert du centre-ville, le parc porte le nom de Théodore Jouvet (1837-1905), le généreux donateur qui a offert à la ville de Valence la somme nécessaire à l'achat du terrain.</p>" },
                    { position: { lat: 44.929222, lng: 4.888631 }, title: 'Kiosque Peynet', description: "<p>Il inspira à <a href='https://fr.wikipedia.org/wiki/Raymond_Peynet'>Raymond Peynet</a> ses célèbres amoureux en 1942.</p>" },
                    { position: { lat: 44.931967, lng: 4.890674 }, title: 'Maison des têtes', description: "<p>Cette maison, marquant le passage du style gothique flamboyant au style renaissance, doit son nom aux nombreuses têtes qui ornent sa façade.</p>" },
                    { position: { lat: 44.931237, lng: 4.889156 }, title: "Musée d'art et d'archéologie", description: "<p>Ses collections, riches de plus de 20 000 œuvres, offrent un large panorama de l’histoire de l'Homme et des arts, de la préhistoire régionale à l’art contemporain.</p>" },
                  ]
                }
              }},
               "Lieux touristiques (Valence)", '']
            else
              ['', '', '']
          end
          BotApi.map(params, title, description).send(session_id)

      when /change(-|\s)?locale/i
        BotApi.card(
          [
            BotApi::Params::build_text('<p>Choose speech to text locale:</p>'),
            BotApi::Params::build_button_group(
              [
                ['fr-FR', { action: 'update_locale', locale: 'fr-FR' }],
                ['en-US', { action: 'update_locale', locale: 'en-US' }],
                ['it-IT', { action: 'update_locale', locale: 'it-IT' }],
                ['ar',    { action: 'update_locale', locale: 'ar' }],
                ['ko-KR', { action: 'update_locale', locale: 'ko-KR' }]
              ],
              false
            )
          ]
        ).send(session_id)

      when /button(-|\s)?(mail)?(-|\s)?(group)?(-|\s)?(deactivable)?/i
        is_mail = !$2.nil? && !$2.empty?
        is_group = !$4.nil? && !$4.empty?
        is_deactivable = !$6.nil? && !$6.empty?
        if is_group
          BotApi.button_group([
            ['Show me a kitten',   { action: 'display_kitten' }],
            ['Show me a puppy',    { action: 'display_puppy'  }],
            ['Show me a duckling', { action: 'display_duckling' }]
          ], is_deactivable).send(session_id)
        else
          if is_mail
            BotApi.button_mail("Contact us", 'mailto:someone@example.com?Subject=Hello%20again').send(session_id)
          else
            random_id = Random.rand(100)
            BotApi.button("Button #{random_id}", { action: "action_#{random_id}"}).send(session_id)
          end
        end

      when /hlist(-|\s)?(card)?/i
        is_card = !$2.nil? && !$2.empty?
        if is_card
          BotApi.list(
            [
              BotApi::Params::build_card([
                BotApi::Params::build_image(BotRessources.kittens[0], 'Lovely kitten - 780$', 'Soooooo cute!'),
                BotApi::Params::build_button('Add to basket', { action: 'kitten_0_added_to_basket' })
              ]),
              BotApi::Params::build_card([
                BotApi::Params::build_image(BotRessources.kittens[1], 'Lovely kitten - 600$', 'Soooooo cute!'),
                BotApi::Params::build_button('Add to basket', { action: 'kitten_1_added_to_basket' })
              ]),
              BotApi::Params::build_card([
                BotApi::Params::build_image(BotRessources.kittens[2], 'Lovely kitten - 1200$', 'Soooooo cute!'),
                BotApi::Params::build_button('Add to basket', { action: 'kitten_2_added_to_basket' })
              ]),
            ],
            :horizontal
          ).send(session_id)
        else
          BotApi
            .list(
              BotRessources.kittens.collect { |img| BotApi::Params::build_image(img) },
              :horizontal
            )
            .add_speech('Here is an horizontal list of kittens', 'en-US')
            .send(session_id)
        end

      when /vlist/i
        BotApi
          .list([
            BotApi::Params::build_image(BotRessources.puppies[0]),
            BotApi::Params::build_image(BotRessources.puppies[1]),
            BotApi::Params::build_text("<strong>Which one is your favorite?</strong>"),
            BotApi::Params::build_button_group([
              ['The first', { action: 'choose_puppy_0' }],
              ['The second', { action: 'choose_puppy_1' }]
            ],
              true)
            ],
            :vertical)
          .add_speech('Here is a vertical list of mixed content', 'en-US')
          .send(session_id)

      when /card(-|\s)?(video)?/i
        is_video = !$2.nil? && !$2.empty?
        if is_video
          BotApi.card([
            BotApi::Params::build_video('bpOSxM0rNPM'),
            BotApi::Params::build_button('Buy the album', { action: 'album_added_to_basket'} )
          ]).send(session_id)
        else
          BotApi.card([
            BotApi::Params::build_image(BotRessources.kittens.sample, 'Lovely kitten - 780$', 'Soooooo cute!'),
            BotApi::Params::build_button('Add to basket', { action: 'album_added_to_basket'})
          ]).send(session_id)
        end

      when /video/i
        description = 'Arctic Monkeys are an English rock band formed in 2002 in High Green'
        description << ', a suburb of Sheffield. Arctic Monkeys new album Tranquility Base '
        description << 'Hotel & Casino is out now on Domino Record Co.'
        BotApi
          .video(
            'bpOSxM0rNPM',
            'Arctic Monkeys - Do I Wanna Know?',
            description
          )
          .add_speech("Let's play music video!", 'en-GB')
          .send(session_id)

        when /geolocation/i
          BotApi.card([
            BotApi::Params::build_text('Do you want to play hide and seek ?'),
            BotApi::Params::build_geolocation('I want to play')
          ]).send(session_id)
      else
        BotApi
          .text("I did not understand: \"#{user_statement_says}\"")
          .add_speech('Oops', 'en-US')
          .send(session_id)
      end
    end

    status 200
    json Hash.new
  end


  get '/ping' do
    base_url = ENV.fetch('VIKYAPP_PUBLIC_URL') { 'http://localhost:3000' }
    url = "#{base_url}/api/v1/ping.json"

    ping_failed = true
    begin
      response = RestClient::Request.execute(
        method: :get,
        url: url,
        headers: { accept: :json },
        timeout: 5
      )
      if response.code == 200
        ping_failed = false
      else
        message = response.body
      end
    rescue RestClient::ExceptionWithResponse => e
      message = ""
      unless e.response.nil?
        message  << "Body: #{e.response.body}"
      end
      message << "Exception message: #{e.message}"
    rescue Exception => e
      message = "Exception message: #{e.message}"
    end

    if ping_failed
      status 500
      json message: message
    else
      status 200
      json Hash.new
    end
  end

  # start the server if ruby file executed directly
  run! if app_file == $0
end
