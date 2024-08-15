#include <rclcpp/rclcpp.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>
#include <nav_msgs/srv/get_map.hpp>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <unistd.h>  // For getcwd

class MapData : public rclcpp::Node {
public:
    MapData() : Node("grid_map_generator") {
        // Initialize publisher, service, and timer
        map_pub_ = this->create_publisher<nav_msgs::msg::OccupancyGrid>("grid_map_viz", 10);
        srv_ = this->create_service<nav_msgs::srv::GetMap>("get_generated_gridmap", std::bind(&MapData::get_map_callback, this, std::placeholders::_1, std::placeholders::_2));
        timer_ = this->create_wall_timer(std::chrono::seconds(1), std::bind(&MapData::master_callback, this));

        // Get file path and read grid map
        std::string file_path = get_grid_map_path();
        if (file_path.empty()) {
            RCLCPP_ERROR(this->get_logger(), "File path is empty. Unable to read grid map.");
            return;
        }

        std::vector<std::vector<int>> data_array = read_grid_map(file_path);
        if (data_array.empty()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to read grid map data. Exiting.");
            return;
        }

        int rows = data_array.size();
        int cols = data_array[0].size();

        // Modify the grid map according to the text file with obstacles and free spaces
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (data_array[i][j] == 1) {
                    data_array[i][j] = 100;  // Obstacle
                } else {
                    data_array[i][j] = 0;  // Free space
                }
            }
        }

        // Set up grid map
        map_data_ = std::make_shared<nav_msgs::msg::OccupancyGrid>();
        map_data_->header.frame_id = "map";
        map_data_->info.resolution = 1.0;
        map_data_->info.width = rows;
        map_data_->info.height = cols;
        map_data_->info.origin.position.x = 0.0;
        map_data_->info.origin.position.y = 0.0;
        map_data_->info.origin.position.z = 0.0;
        map_data_->info.origin.orientation.x = 0.0;
        map_data_->info.origin.orientation.y = 0.0;
        map_data_->info.origin.orientation.z = 0.0;
        map_data_->info.origin.orientation.w = 1.0;

        // Flatten the 2D array and assign it to map_data_
        map_data_->data.reserve(rows * cols);
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                map_data_->data.push_back(data_array[i][j]);
            }
        }

        RCLCPP_INFO(this->get_logger(), "Map data successfully loaded and processed.");
    }

private:
    rclcpp::Publisher<nav_msgs::msg::OccupancyGrid>::SharedPtr map_pub_;
    rclcpp::Service<nav_msgs::srv::GetMap>::SharedPtr srv_;
    rclcpp::TimerBase::SharedPtr timer_;
    nav_msgs::msg::OccupancyGrid::SharedPtr map_data_;

    std::string get_grid_map_path() {
        char buffer[256];
        if (getcwd(buffer, sizeof(buffer)) != nullptr) {
            std::string path(buffer);
            path += "/map_viz/maps/grid_map.txt";
            return path;
        }
        return "";
    }

    std::vector<std::vector<int>> read_grid_map(const std::string& file_path) {
        std::vector<std::vector<int>> grid_map;
        std::ifstream file(file_path);
        if (!file.is_open()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to open file: %s", file_path.c_str());
            return grid_map;
        }
        std::string line;
        while (std::getline(file, line)) {
            std::vector<int> row;
            std::istringstream iss(line);
            int value;
            while (iss >> value) {
                row.push_back(value);
            }
            grid_map.push_back(row);
        }
        return grid_map;
    }

    void get_map_callback(const std::shared_ptr<nav_msgs::srv::GetMap::Request> request,
                          std::shared_ptr<nav_msgs::srv::GetMap::Response> response) {
        RCLCPP_INFO(this->get_logger(), "Map requested");
        response->map = *map_data_;
        RCLCPP_INFO(this->get_logger(), "Map sent");
    }

    void master_callback() {
        // Publish the grid_map data
        if (map_data_) {
            map_pub_->publish(*map_data_);
            RCLCPP_INFO(this->get_logger(), "Publishing map data");
        } else {
            RCLCPP_WARN(this->get_logger(), "Map data is not initialized.");
        }
    }
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<MapData>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
