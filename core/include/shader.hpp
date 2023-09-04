#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <glm.hpp>

class ShaderGL
{
public:
	unsigned int id;
	
	ShaderGL() = default;
	~ShaderGL();

	bool init(const std::string& vertex_path, const std::string& fragment_path);
	void use();

	void set_bool(const std::string& name, bool value) const;
	void set_int(const std::string& name, int value) const;
	void set_float(const std::string& name, float value) const;
	void set_mat4(const std::string& name, const glm::mat4& value) const;
};