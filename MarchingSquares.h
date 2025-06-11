// Code needs to be rewritten to exclude functions from the header (better approach to life)

/**
 * A simple implementation of the marching squares algorithm that can identify
 * perimeters in an supplied byte array. The array of data over which this
 * instances of this class operate is not cloned by this class's constructor
 * (for obvious efficiency reasons) and should therefore not be modified while
 * the object is in use. It is expected that the data elements supplied to the
 * algorithm have already been thresholded. The algorithm only distinguishes
 * between zero and non-zero values.
 *
 * @author Tom Gibara
 * Ported to C++ by Juha Reunanen
 *
 */

#pragma once

#include "Direction.h"
#include "Utility.h"
#include <vector>
#include <sstream>
#include <queue>

namespace MarchingSquares {

    struct Result {
        int initialX = -1;
        int initialY = -1;
        std::vector<Direction> directions;
    };

    /**
     * Finds the perimeter between a set of zero and non-zero values which
     * begins at the specified data element. If no initial point is known,
     * consider using the convenience method supplied. The paths returned by
     * this method are always closed.
     *
     * The length of the supplied data array must exceed width * height,
     * with the data elements in row major order and the top-left-hand data
     * element at index zero.
     *
     * @param initialX
     *            the column of the data matrix at which to start tracing the
     *            perimeter
     * @param initialY
     *            the row of the data matrix at which to start tracing the
     *            perimeter
     * @param width
     *            the width of the data matrix
     * @param height
     *            the width of the data matrix
     * @param data
     *            the data elements
     *
     * @return a closed, anti-clockwise path that is a perimeter of between a
     *         set of zero and non-zero values in the data.
     * @throws std::runtime_error
     *             if there is no perimeter at the specified initial point.
     */

    inline
    Result FindPerimeter(int initialX, int initialY, int width, int height, unsigned char* data) {
        if (initialX < 0) initialX = 0;
        if (initialX > width) initialX = width;
        if (initialY < 0) initialY = 0;
        if (initialY > height) initialY = height;

        int initialValue = value(initialX, initialY, width, height, data);
        if (initialValue == 0 || initialValue == 15) {
            std::ostringstream error;
            error << "Supplied initial coordinates (" << initialX << ", " << initialY << ") do not lie on a perimeter.";
            throw std::runtime_error(error.str());
        }

        Result result;

        int x = initialX;
        int y = initialY;
        Direction previous = MakeDirection(0, 0);

        do {
            Direction direction;
            switch (value(x, y, width, height, data)) {
                case  1: direction = North(); break;
                case  2: direction = East(); break;
                case  3: direction = East(); break;
                case  4: direction = West(); break;
                case  5: direction = North(); break;
                case  6: direction = previous == North() ? West() : East(); break;
                case  7: direction = East(); break;
                case  8: direction = South(); break;
                case  9: direction = previous == East() ? North() : South(); break;
                case 10: direction = South(); break;
                case 11: direction = South(); break;
                case 12: direction = West(); break;
                case 13: direction = North(); break;
                case 14: direction = West(); break;
                default: throw std::runtime_error("Illegal state");
            }
            if (direction == previous) {
                // compress
                result.directions.back() += direction;
            }
            else {
                result.directions.push_back(direction);
                previous = direction;
            }
            x += direction.x;
            y -= direction.y; // accommodate change of basis
        } while (x != initialX || y != initialY);

        result.initialX = initialX;
        result.initialY = initialY;

        return result;
    }

    /**
     * A convenience method that locates at least one perimeter in the data with
     * which this object was constructed. If there is no perimeter (ie. if all
     * elements of the supplied array are identically zero) then null is
     * returned.
     *
     * @return a perimeter path obtained from the data, or null
     */
    inline
    Result FindPerimeter(int width, int height, unsigned char* data) {
        int size = width * height;
        for (int i = 0; i < size; i++) {
            if (data[i] != 0) {
                return FindPerimeter(i % width, i / width, width, height, data);
            }
        }
        Result result;
        return result;
    }

    inline
    int FloodFill(int width, int height, unsigned char* data, std::vector<bool>& visited, const Result& perimeter) {
        int x = perimeter.initialX;
        int y = perimeter.initialY;
        int filled = 0;

        for (const Direction& dir : perimeter.directions) {
            int inX = x + dir.y;
            int inY = y - dir.x;

            if (inX >= 0 && inX < width && inY >= 0 && inY < height) {

                int idx = inY * width + inX;
                if (data[idx] != 0 && !visited[idx]) {
                    std::queue<std::pair<int, int>> q;
                    q.push({inX, inY});

                    while (!q.empty()) {
                        auto [cx, cy] = q.front();
                        q.pop();

                        if (cx < 0 || cx >= width || cy < 0 || cy >= height) continue;

                        int cidx = cy * width + cx;
                        if (visited[cidx] || data[cidx] == 0) continue;

                        visited[cidx] = true;
                        filled++;

                        q.push({cx + 1, cy});
                        q.push({cx - 1, cy});
                        q.push({cx, cy + 1});
                        q.push({cx, cy - 1});
                    }

                    break;
                }
            }

            x += dir.x;
            y -= dir.y;
        }
        return filled;
    }


    inline
    std::vector<Result> FindPerimeters(int width, int height, int minSize, unsigned char* data) {
        std::vector<Result> results;
        std::vector<bool> visited(width * height, false);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = y * width + x;
                if (data[idx] == 0 || visited[idx])
                    continue;
                try {
                    Result perimeter = FindPerimeter(x, y, width, height, data);

                    // Mark perimeter pixels as visited
                    int px = perimeter.initialX;
                    int py = perimeter.initialY;
                    for (const Direction& dir : perimeter.directions) {
                        if (px >= 0 && px < width && py >= 0 && py < height) {
                            visited[py * width + px] = true;
                        }
                        px += dir.x;
                        py -= dir.y;
                    }

                    int size = FloodFill(width, height, data, visited, perimeter);
                    if (size >= minSize) {
                        results.push_back(perimeter);

                    }
                } catch (const std::runtime_error&) {
                    // If no perimeter found at this point, continue to next cell
                }
            }
        }
        return results;
    }

}
