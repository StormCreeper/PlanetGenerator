#pragma once

#include <glad/glad.h>

class Framebuffer {
   public:
	Framebuffer(int width, int height) : m_width(width), m_height(height) {}
	~Framebuffer() { glDeleteFramebuffers(1, &m_fbo); }

	void init() {
		glGenFramebuffers(1, &m_fbo);
		bind();

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGB,
					 GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glFramebufferTexture2D(GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0,
							   GL_TEXTURE_2D, m_texture, 0);
	}

	void bind() { glBindFramebuffer(GL_FRAMEBUFFER, m_fbo); }
	void unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

   private:
	GLuint m_fbo;
	GLuint m_texture;
	int m_width;
	int m_height;
};