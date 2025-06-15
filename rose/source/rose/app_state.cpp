#include <rose/app_state.hpp>

#include <random>

void AppState::init()
{ 
    // construct kernel for SSAO
	std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<f32> dist(0.0f, 1.0f);
    ssao_kernel.resize(64);

    for (u32 idx = 0; idx < ssao_kernel.size(); ++idx) {
        // hemispherical samples
        // [ -1, 1 ], [ -1, 1 ], [ 0, 1 ]
        glm::vec4 sample = glm::vec4(dist(rng) * 2.0f - 1.0f, dist(rng) * 2.0f - 1.0f, dist(rng), 1.0f);
        sample = glm::normalize(sample);
        sample *= dist(rng);
        // distributing points more densely towards origin
        f32 scale = ((f32)idx) / ((f32)ssao_kernel.size());
        scale = std::lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        sample.a = 1.0f;
        ssao_kernel[idx] = sample;
    }

	return;
}