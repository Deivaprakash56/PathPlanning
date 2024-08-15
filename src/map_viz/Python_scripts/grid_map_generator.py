#!/usr/bin/env python3

import rclpy
import os
from rclpy.node import Node
from nav_msgs.msg import OccupancyGrid
from nav_msgs.srv import GetMap
import numpy as np

class MapData(Node):

    def __init__(self):
        super().__init__('Grid_map_generator')
        self.map_pub = self.create_publisher(OccupancyGrid, 'grid_map_viz', 10)
        self.srv = self.create_service(GetMap, 'get_generated_gridmap', self.get_map_callback)

        self.timer = self.create_timer(1, self.master_callback)

        script_directory = os.path.dirname(os.path.abspath(__file__))
        file_path = os.path.join(script_directory, 'grid_map.txt')

        data_array = self.read_grid_map(file_path)

        # Modifying the grid map as per the text file with obstacles and free spaces
        rows, cols = data_array.shape

        for i in range(rows):
            for j in range(cols):
                if data_array[i, j] == 1:
                    data_array[i, j] = 100  # Obstacle
                else:
                    data_array[i, j] = 0  # Free space


        # grid map
        self.map_data = OccupancyGrid()
        self.map_data.header.frame_id = 'map'
        self.map_data.info.resolution = 1.0
        self.map_data.info.width = rows
        self.map_data.info.height = cols
        self.map_data.info.origin.position.x = 0.0
        self.map_data.info.origin.position.y = 0.0
        self.map_data.info.origin.position.z = 0.0
        self.map_data.info.origin.orientation.x = 0.00
        self.map_data.info.origin.orientation.y = 0.00
        self.map_data.info.origin.orientation.z = 0.00
        self.map_data.info.origin.orientation.w = 1.00

        # Convert 2D array to a single list and assign to map_data
        self.map_data.data = data_array.flatten().tolist()

    def read_grid_map(self, file_path):
        grid_map = []
        with open(file_path, 'r') as file:
            for line in file:
                row = [int(char) for char in line.split()]
                grid_map.append(row)
        return np.array(grid_map)

    def get_map_callback(self, request, response):
        # Generate and return the map
        self.get_logger().info(f"{request}")
        response.map = self.map_data
        self.get_logger().info("Map received")
        return response

    def master_callback(self):
        # Publish the grid_map data
        self.map_pub.publish(self.map_data)

def main(args=None):
    rclpy.init(args=args)
    map_viz_node = MapData()
    rclpy.spin(map_viz_node)
    map_viz_node.destroy_node()
    rclpy.shutdown()

if __name__ == "__main__":
    main()
