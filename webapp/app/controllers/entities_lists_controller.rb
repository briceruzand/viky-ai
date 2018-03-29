class EntitiesListsController < ApplicationController
  skip_before_action :verify_authenticity_token, only: [:update_positions]
  before_action :set_owner
  before_action :set_agent
  before_action :check_user_rights
  before_action :set_entities_list, except: [:index, :new, :create, :update_positions]

  def index
    @entities_lists = @agent.entities_lists.order('position desc, created_at desc')
  end

  def show
    respond_to do |format|
      format.html { @entity = Entity.new }
      format.csv do
        filename = "#{@owner.username}_#{@agent.agentname}_#{@entities_list.listname}_#{Time.current.strftime('%Y-%m-%d')}.csv"
        response.headers['Content-Disposition'] = 'attachment; filename="' + filename + '"'
        render :show
      end
    end
  end

  def new
    @entities_list = EntitiesList.new(visibility: EntitiesList.visibilities.key(EntitiesList.visibilities[:is_private]))
    render partial: 'new'
  end

  def create
    @entities_list = EntitiesList.new(entities_list_params)
    @entities_list.agent = @agent
    respond_to do |format|
      if @entities_list.save
        format.json do
          redirect_to user_agent_entities_lists_path(current_user, @agent), notice: t('views.entities_lists.new.success_message')
        end
      else
        format.json do
          render json: {
            replace_modal_content_with: render_to_string(partial: 'new', formats: :html)
          }, status: 422
        end
      end
    end
  end

  def edit
    render partial: 'edit'
  end

  def update
    respond_to do |format|
      if @entities_list.update(entities_list_params)
        format.json {
          redirect_to user_agent_entities_lists_path(current_user, @agent), notice: t('views.entities_lists.edit.success_message')
        }
      else
        format.json {
          render json: {
            replace_modal_content_with: render_to_string(partial: 'edit', formats: :html),
          }, status: 422
        }
      end
    end
  end

  def update_positions
    EntitiesList.update_positions(@agent, params[:is_public], params[:is_private])
  end

  def confirm_destroy
    render partial: 'confirm_destroy'
  end

  def destroy
    if @entities_list.destroy
      redirect_to user_agent_entities_lists_path(current_user, @agent), notice: t(
        'views.entities_lists.destroy.success_message', name: @entities_list.listname
      )
    else
      redirect_to user_agent_entities_lists_path(current_user, @agent), alert: t(
        'views.entities_lists.destroy.errors_message',
        errors: @entities_list.errors.full_messages.join(', ')
      )
    end
  end

  def move_to_agent
    user_destination = User.friendly.find(params[:user])
    agent_destination = user_destination.agents.friendly.find(params[:agent])
    if @entities_list.change_agent(current_user, agent_destination)
      redirect_to user_agent_entities_lists_path(@owner, @agent), notice: t(
        'views.entities_lists.move_to.success_message',
        name: @entities_list.listname,
        agent: agent_destination.agentname
      )
    else
      redirect_to user_agent_entities_lists_path(@owner, @agent), alert: t(
        'views.entities_lists.move_to.errors_message',
        errors: @entities_list.errors.full_messages.join(', ')
      )
    end
  end


  private

    def set_entities_list
      entities_list_id = params[:entities_list_id] || params[:id]
      @entities_list = @agent.entities_lists.friendly.find(entities_list_id)
    end

    def entities_list_params
      params.require(:entities_list).permit(:listname, :description, :visibility)
    end

    def set_owner
      @owner = User.friendly.find(params[:user_id])
    end

    def set_agent
      @agent = @owner.agents.friendly.find(params[:agent_id])
    end

    def check_user_rights
      case action_name
        when 'show', 'index'
          access_denied unless current_user.can? :show, @agent
        when 'new', 'create', 'edit', 'update', 'confirm_destroy', 'destroy', 'update_positions', 'available_destinations', 'move_to_agent'
          access_denied unless current_user.can? :edit, @agent
        else
          access_denied
      end
    end
end
