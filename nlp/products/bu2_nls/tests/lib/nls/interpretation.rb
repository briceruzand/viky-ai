# encoding: utf-8

module Nls

  class Interpretation

    @@default_locale = "fr-FR"
    attr_reader :id
    attr_reader :slug
    attr_reader :expressions
    attr_accessor :solution
    attr_accessor :package

    def initialize(slug, opts = {})

      @slug = slug

      # default values
      opts[:id] = UUIDTools::UUID.timestamp_create if !opts.has_key?(:id)

      @id = opts[:id]

      @expressions = []

      @solution = nil
      @solution = opts[:solution] if opts.has_key?(:solution)

    end

    def add_expression(new_expression)

      if !new_expression.kind_of? Expression
        raise "Expression (#{new_expression}, #{new_expression.class}) added must a #{Expression.name} in interpretation (#{@package.slug}/#{@slug})"
      end

      @expressions << new_expression
      new_expression.interpretation = self
    end
    alias_method '<<', 'add_expression'

    def to_h
      hash = {}
      hash["id"] = @id.to_s
      hash["slug"] = @slug
      hash["expressions"] = @expressions.map{|v| v.to_h}
      if !@solution.nil?
        if @solution.respond_to?(:to_h)
          hash['solution'] = @solution.to_h
        else
          hash['solution'] = @solution
        end
      end
      hash
    end

    def to_match(score = 1.0)
      {
        "package" => package.id.to_s,
        "id" => @id.to_s ,
        "slug" => @slug,
        "score" => score
      }
    end

    def to_json(options = {})
      to_h.to_json(options)
    end

    def new_expression(text, opts = {} )
      aliases = []
      aliases = opts[:aliases] if opts.has_key?(:aliases)
      locale = nil
      locale = opts[:locale] if opts.has_key?(:locale)
      keep_order = false
      keep_order = opts[:keep_order] if opts.has_key?(:keep_order)
      solution = nil
      solution = opts[:solution] if opts.has_key?(:solution)
      add_expression(Expression.new(text, {aliases: aliases, locale: locale, keep_order: keep_order, solution: solution}))
      self
    end

    def new_textual(texts = [], opts = {})
      locale = @@default_locale
      locale = opts[:locale] if opts.has_key?(:locale)
      glued = false
      glued = opts[:glued] if opts.has_key?(:glued)
      keep_order = false
      keep_order = opts[:keep_order] if opts.has_key?(:keep_order)
      solutions = nil
      solutions = opts[:solutions] if opts.has_key?(:solutions)
      if texts.kind_of? Array
        texts.each do |t|
          add_expression(Expression.new(t, {locale: locale, glued: glued, keep_order: keep_order, solutions: solutions}))
        end
      else
        add_expression(Expression.new(texts, {locale: locale, glued: glued, keep_order: keep_order, solutions: solutions}))
      end
      self
    end

    def self.default_locale=(locale)
      @@default_locale = locale
    end

    def self.default_locale
      @@default_locale
    end

  end
end

