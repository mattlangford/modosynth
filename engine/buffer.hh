
#include <Eigen/Dense>
#include <variant>
#include <vector>

namespace engine {

//
// #############################################################################
//

struct Point {
    Eigen::Vector2f point;
};

struct Line {
    Eigen::Vector2f start_point;
    Eigen::Vector2f end_point;
};

struct Triangle {
    Eigen::Vector2f p0;
    Eigen::Vector2f p1;
    Eigen::Vector2f p2;
};

struct Quad {
    Eigen::Vector2f top_left;
    Eigen::Vector2f top_right;
    Eigen::Vector2f bottom_left;
    Eigen::Vector2f bottom_right;
};

using Primitive = std::variant<Point, Line, Triangle, Quad>;

//
// #############################################################################
//

class Buffer2Df {
public:
    Buffer2Df();
    ~Buffer2Df();

public:
    void init(int location);

public:
    size_t add(const Primitive& primitive);
    void update(const Primitive& primitive, size_t& index);

private:
    void reset_buffers();

public:
    unsigned int vertex_buffer_ = -1;
    unsigned int element_buffer_ = -1;

    std::vector<Eigen::Vector2f> vertices_;
    std::vector<unsigned int> indices_;
};
}  // namespace engine
