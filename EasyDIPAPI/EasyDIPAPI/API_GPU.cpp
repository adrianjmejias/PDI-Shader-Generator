#include "API_GPU.h"


namespace ED {
	std::unique_ptr<Quad> quad;

	std::unique_ptr<Shader> greyShader;
	std::unique_ptr<Shader> bwShader;
	std::unique_ptr<Shader> negativeShader;
	std::string vert;

	std::unique_ptr<Shader> sobelShader;
	std::unique_ptr<Shader> roberts;
	std::unique_ptr<Shader> prewitt;
	std::unique_ptr<Shader> box;
	std::unique_ptr<Shader> median;
	std::unique_ptr<Shader> laplaceGauss;

	RawData* ApplySobelHA(PARAMS_LOCAL) {

		auto [convX, newWidthX, newHeightX] = ReduceConvolution(sobelX, meta[0]);
		auto [convY, newWidthY, newHeightY] = ReduceConvolution(sobelY, meta[1]);

				std::string sobel = BuildShaderConv(
					//uniforms
					UseGradient(),
					// before
					BuildConvolution(convX, "convX") +
					BuildConvolution(convY, "convY") +
					"vec3 avgX = vec3(0);\n",
					// op
					UseForConv(meta[0],
							"vec3 color = texture(tex, nUv).rgb;\n"
							"avgX +=  color * conv[convI];\n"			
						) +
					UseForConv(meta[1],
						"vec3 color = texture(tex, nUv).rgb;\n"
						"avgX +=  color * conv[convI];\n"
						),
					// after
					"fragColor = vec4(avgX, 1);\n"
				);


				sobelShader.reset(Shader::FromString(vert.c_str(), sobel.c_str()));




		return ApplyConvolutionHA(data, imgWidth, imgHeight, nChannels, *sobelShader);
	}

	RawData* ApplyNegativeHA(PARAMS_GLOBAL) {
		return ApplyConvolutionHA(data, width, height, nChannels, *negativeShader);
	}

	RawData* ApplyConvolutionHA(RawData* data, unsigned int imgWidth, unsigned int imgHeight, unsigned int nChannels, Shader& s)
	{
		// The texture we're going to render to
		GLint m_viewport[4];

		glGetIntegerv(GL_VIEWPORT, m_viewport);

		std::cout << "viewport" << std::endl;

		//for (int ii = 0; ii < 4; ii++)
		//{
		//	std::cout << m_viewport[ii] << std::endl;
		//}

		GLuint renderedTexture = GetTexture(nullptr, imgWidth, imgHeight);

		GLuint FramebufferName = 0;
		glGenFramebuffers(1, &FramebufferName);
		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderedTexture, 0);
		GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			__debugbreak();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		glViewport(0, 0, imgWidth, imgHeight);
		glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
									   //PreApplyConvolution(tex);
		RawData* out;
		unsigned int texture;
		texture = GetTexture(data, imgWidth, imgHeight);

		// tratar de hacer que el user pase el out que quiera usar
		out = new RawData[(imgWidth * imgHeight) * nChannels];

		s.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);

		s.setInt("tex", 0);
		s.setFloat("imgWidth", imgWidth);
		s.setFloat("imgHeight", imgHeight);

		quad->Bind();
		quad->Draw();

		glBindTexture(GL_TEXTURE_2D, renderedTexture);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, out);
		glDeleteTextures(1, &texture);
		glDeleteTextures(1, &renderedTexture);

		glViewport(0, 0, m_viewport[2], m_viewport[3]);
		return out;
	}

	bool EDInitGPU()
	{

		quad.reset(new Quad());
		vert = Shader::GetSrcFromFile("bw.vert");

		std::string fragNegative = BuildGlobalShader(
			"fragColor = 1 - fragColor;"
		);

		std::string fragGreyscale = BuildGlobalShader(
			//"fragColor = 1 - fragColor;"
			"fragColor.rgb = SU_GREYSCALE(fragColor.rgb);",
			UseGreyScale()
		);

		std::string fragBW = BuildGlobalShader(
			//"fragColor = 1 - fragColor;"
			"fragColor.rgb = SU_BW(fragColor.rgb, 150);",
			UseBW()
		);


		std::vector<float> sobelX = {
			1, 2, 1,
			0, 0, 0,
			-1, -2, -1,
		};
		std::vector<float> sobelY = {
			-1 , 0, 1,
			-2 , 0, 2,
			-1 , 0, 1,
		};





		std::cout << "grey shader" << std::endl;
		greyShader.reset(Shader::FromString(vert.c_str(), fragGreyscale.c_str()));

		std::cout << "bw shader" << std::endl;
		bwShader.reset(Shader::FromString(vert.c_str(), fragBW.c_str()));
		std::cout << "negative shader" << std::endl;

		negativeShader.reset(Shader::FromString(vert.c_str(), fragNegative.c_str()));

		return true;
	}
}