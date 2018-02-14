require "application_system_test_case"
require 'model_test_helper'

class AgentsDependenciesTest < ApplicationSystemTestCase


  test "Add agent dependency" do
    go_to_agents_index
    click_link "My awesome weather bot admin/weather"
    assert page.has_text?('Agents / My awesome weather bot (admin/weather)')
    assert page.has_text?(' Agent dependencies (0) - Dependents (0) ')

    # Add admin/terminator to admin/weather
    click_link "Add new dependency"
    within(".modal") do
      assert_equal ["T-800 admin/terminator"], all('a').collect(&:text)
      click_link "T-800 admin/terminator"
    end

    assert page.has_text?('T-800 admin/terminator')
    assert page.has_no_text?('Add new dependency')
    assert page.has_text?(' Agent dependencies (1) - Dependents (0) ')

    # Try to add admin/weather to admin/terminator and detect cycle
    click_link "T-800 admin/terminator"
    assert page.has_text?('Agents / T-800 (admin/terminator)')
    click_link "Add new dependency"

    within(".modal") do
      assert_equal ["My awesome weather bot admin/weather"], all('a').collect(&:text)
      click_link "My awesome weather bot admin/weather"
    end

    assert page.has_text?('agent addition would cause a cycle in dependency graph')
  end


  test "Use alias from successor agent and delete dependency" do
    go_to_agents_index
    click_link "My awesome weather bot admin/weather"
    assert page.has_text?('Agents / My awesome weather bot (admin/weather)')
    assert page.has_text?(' Agent dependencies (0) - Dependents (0) ')

    # Add dependency
    click_link "Add new dependency"
    within(".modal") do
      assert_equal ["T-800 admin/terminator"], all('a').collect(&:text)
      click_link "T-800 admin/terminator"
    end
    assert page.has_text?('T-800 admin/terminator')

    # Delete ? no
    click_link "Delete"
    within(".modal") do
      assert page.has_text?('No reference to agent "T-800" interpretations')
      click_link "Cancel"
    end

    # Edit an interpretation
    click_link 'weather_forecast'
    assert page.has_text?('weather_forecast PUBLIC (admin/weather/weather_forecast)')

    # Add Expression & create an alias from dependency
    first('trix-editor').click.set('Salut Marcel')
    select_text_in_trix("trix-editor", 6, 12)
    find_link('admin/terminator/terminator_find').click
    click_button 'Add'

    assert page.has_text?("Salut Marcel")
    assert_equal 2, all('.interpretation-resume').count
    assert_equal 1, all('span[title="admin/terminator/terminator_find"]').size

    # Return to interpretation list
    click_link 'My awesome weather bot'

    # Delete dependency
    click_link "Delete"
    within(".modal") do
      assert page.has_text?('One reference to agent "T-800" interpretations')
      fill_in 'validation', with: 'DELETE'
      click_button('Delete')
    end
    assert page.has_no_text?('T-800 admin/terminator')

    # Edit an interpretation
    click_link 'weather_forecast'
    assert page.has_text?("Salut Marcel")
    assert_equal 1, all('.interpretation-resume').count
    assert_equal 0, all('span[title="admin/terminator/terminator_find"]').size
  end


  test 'Cannot add dependency if no edit right' do
    create_agent('dependency_test', :show_on_agent_weather)

    login_as 'show_on_agent_weather@viky.ai', 'BimBamBoom'
    assert page.has_text?('Agents')
    click_link 'My awesome weather bot admin/weather'
    assert page.has_text?('Agents / My awesome weather bot (admin/weather)')
    assert page.has_no_link?('Add new dependency')
  end
end
