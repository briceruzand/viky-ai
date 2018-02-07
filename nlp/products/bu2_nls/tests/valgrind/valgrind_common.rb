require 'test_helper'

module Valgrind

  class ValgrindCommon < Nls::NlsTestCommon

    def setup

      resetDir

      Nls::Interpretation.default_locale = "en-GB"

    end

    def interpret_queries(nb_request_factor, skip_timeout = false)

      # several_packages_several_intents
      pg_building_feature_any = create_building_feature_any
      Nls::Nls.package_update(pg_building_feature_any)

      numbers_package = create_numbers_package
      Nls::Nls.package_update(numbers_package)

      @main_package = @available_packages["pg-building-feature"]
      @main_uuid = @main_package.id.to_s

      @number_package = @available_packages["numbers"]
      @number_uuid = @number_package.id.to_s

      interpret_query=
      {
        "packages" => [ @main_uuid ],
        "sentence" => "with swimming pool with golf",
        "Accept-Language" => Nls::Interpretation.default_locale
      }

      numbers_query=
      {
        "packages" => [ @number_uuid ],
        "sentence" => "a 12 345.678 b",
        "Accept-Language" => Nls::Interpretation.default_locale
      }

      expected_error = "NlsCancelCleanupOnTimeout : Request timeout after"

      params = {
        timeout: 20000,
      }

      # dump packages
      (10 * nb_request_factor).times do |i|
        actual_dump_result = Nls::Nls.dump
        assert !actual_dump_result.nil?, "dump #{i}"
      end

      # launch simple query
      (10 * nb_request_factor).times do
        response = Nls::Nls.interpret(interpret_query, params)
        assert !response.nil?
      end

      # launch number query
      (10 * nb_request_factor).times do
        response = Nls::Nls.interpret(numbers_query, params)
        assert !response.nil?
      end

      # package update
      url_add = Nls::Nls.url_packages + "/#{@main_uuid}"

      expected_update_result =
      {
        "status" => "Package '#{@main_uuid}' successfully updated"
      }

      (10 * nb_request_factor).times do |i|
        actual_update_result = Nls::Nls.query_post(url_add, @main_package)
        assert_json expected_update_result, actual_update_result, "updating #{i}"
      end

      # launch queries with timeouts

      if !skip_timeout

        # timeout NlpPackageGet for nlp recursive query
        params = {
          timeout: 20,
          timeout_in: "NlpPackageGet"
        }

        (10 * nb_request_factor).times do |i|
          exception = assert_raises RestClient::ExceptionWithResponse do
            Nls::Nls.interpret(interpret_query, params)
          end
          assert_response_has_error expected_error, exception, "Timeout #{i}"
        end

        # timeout NlpInterpretRequestParse
        params = {
          timeout: 20,
          timeout_in: "NlpInterpretRequestParse"
        }

        (10 * nb_request_factor).times do |i|
          exception = assert_raises RestClient::ExceptionWithResponse do
            Nls::Nls.interpret(interpret_query, params)
          end
          assert_response_has_error expected_error, exception, "Timeout #{i}"
        end

        # timeout NlpMatchExpressions
        params = {
          timeout: 20,
          timeout_in: "NlpMatchExpressions"
        }

        (10 * nb_request_factor).times do |i|
          exception = assert_raises RestClient::ExceptionWithResponse do
            Nls::Nls.interpret(interpret_query, params)
          end
          assert_response_has_error expected_error, exception, "Timeout #{i}"
        end

        # timeout NlpRequestInterpretationBuild
        params = {
          timeout: 20,
          timeout_in: "NlpRequestInterpretationBuild"
        }

        (10 * nb_request_factor).times do |i|
          exception = assert_raises RestClient::ExceptionWithResponse do
            Nls::Nls.interpret(interpret_query, params)
          end
          assert_response_has_error expected_error, exception, "Timeout #{i}"
        end

        # timeout NlpPackageAddOrReplace
        params = {
          timeout: 20,
          timeout_in: "NlpPackageAddOrReplace"
        }

        (10 * nb_request_factor).times do |i|
          exception = assert_raises RestClient::ExceptionWithResponse do
            Nls::Nls.package_update(@main_package, params)
          end
          assert_response_has_error expected_error, exception, "Timeout #{i}"
        end

      end

    end

  end
end
