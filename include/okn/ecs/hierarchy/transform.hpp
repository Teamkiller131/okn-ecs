#pragma once

#include <cmath>

namespace okn::ecs {

struct LocalTransform {
    float pos[3] = {0.0f, 0.0f, 0.0f};
    float rot[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float scl[3] = {1.0f, 1.0f, 1.0f};
};

struct WorldTransform {
    float pos[3] = {0.0f, 0.0f, 0.0f};
    float rot[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float scl[3] = {1.0f, 1.0f, 1.0f};
    float mat[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
};

namespace transform_detail {

inline void set_identity(float m[16]) {
    m[0] = 1.0f; m[1] = 0.0f; m[2] = 0.0f; m[3] = 0.0f;
    m[4] = 0.0f; m[5] = 1.0f; m[6] = 0.0f; m[7] = 0.0f;
    m[8] = 0.0f; m[9] = 0.0f; m[10]= 1.0f; m[11]= 0.0f;
    m[12]= 0.0f; m[13]= 0.0f; m[14]= 0.0f; m[15]= 1.0f;
}

inline void mul_mat4(const float a[16], const float b[16], float out[16]) {
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            out[col * 4 + row] =
                a[0 * 4 + row] * b[col * 4 + 0] +
                a[1 * 4 + row] * b[col * 4 + 1] +
                a[2 * 4 + row] * b[col * 4 + 2] +
                a[3 * 4 + row] * b[col * 4 + 3];
        }
    }
}

inline void quat_to_mat4(const float q[4], float m[16]) {
    float x = q[0], y = q[1], z = q[2], w = q[3];
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;

    m[0] = 1.0f - 2.0f * (yy + zz);
    m[1] = 2.0f * (xy + wz);
    m[2] = 2.0f * (xz - wy);
    m[3] = 0.0f;

    m[4] = 2.0f * (xy - wz);
    m[5] = 1.0f - 2.0f * (xx + zz);
    m[6] = 2.0f * (yz + wx);
    m[7] = 0.0f;

    m[8] = 2.0f * (xz + wy);
    m[9] = 2.0f * (yz - wx);
    m[10]= 1.0f - 2.0f * (xx + yy);
    m[11]= 0.0f;

    m[12]= 0.0f;
    m[13]= 0.0f;
    m[14]= 0.0f;
    m[15]= 1.0f;
}

inline void translate_mat4(float m[16], float tx, float ty, float tz) {
    m[12] += tx;
    m[13] += ty;
    m[14] += tz;
}

inline void scale_mat4(float m[16], float sx, float sy, float sz) {
    m[0] *= sx;  m[1] *= sx;  m[2] *= sx;  m[3] *= sx;
    m[4] *= sy;  m[5] *= sy;  m[6] *= sy;  m[7] *= sy;
    m[8] *= sz;  m[9] *= sz;  m[10]*= sz;  m[11]*= sz;
}

inline void compose_transform(const LocalTransform& local, float result[16]) {
    set_identity(result);
    quat_to_mat4(local.rot, result);
    translate_mat4(result, local.pos[0], local.pos[1], local.pos[2]);
    scale_mat4(result, local.scl[0], local.scl[1], local.scl[2]);
}

} // namespace transform_detail

inline auto compute_world_matrix(const LocalTransform& local, const float* parent_world = nullptr) -> WorldTransform {
    WorldTransform wt{};
    float local_mat[16];
    transform_detail::compose_transform(local, local_mat);

    if (parent_world != nullptr) {
        transform_detail::mul_mat4(parent_world, local_mat, wt.mat);
    } else {
        for (int i = 0; i < 16; ++i) {
            wt.mat[i] = local_mat[i];
        }
    }

    wt.pos[0] = wt.mat[12];
    wt.pos[1] = wt.mat[13];
    wt.pos[2] = wt.mat[14];

    wt.rot[0] = local.rot[0];
    wt.rot[1] = local.rot[1];
    wt.rot[2] = local.rot[2];
    wt.rot[3] = local.rot[3];

    wt.scl[0] = local.scl[0];
    wt.scl[1] = local.scl[1];
    wt.scl[2] = local.scl[2];

    return wt;
}

inline auto decompose_matrix(const float m[16]) -> WorldTransform {
    WorldTransform wt{};
    for (int i = 0; i < 16; ++i) {
        wt.mat[i] = m[i];
    }
    wt.pos[0] = m[12];
    wt.pos[1] = m[13];
    wt.pos[2] = m[14];

    wt.rot[0] = 0.0f;
    wt.rot[1] = 0.0f;
    wt.rot[2] = 0.0f;
    wt.rot[3] = 1.0f;

    float col0_len = std::sqrt(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
    float col1_len = std::sqrt(m[4] * m[4] + m[5] * m[5] + m[6] * m[6]);
    float col2_len = std::sqrt(m[8] * m[8] + m[9] * m[9] + m[10]* m[10]);

    wt.scl[0] = col0_len;
    wt.scl[1] = col1_len;
    wt.scl[2] = col2_len;

    return wt;
}

} // namespace okn::ecs
