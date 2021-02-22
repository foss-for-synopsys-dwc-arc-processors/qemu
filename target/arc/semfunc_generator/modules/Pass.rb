module Pass

  def name
    return self.class
  end

  def execute(ast)
    return task(ast)
  end

end
