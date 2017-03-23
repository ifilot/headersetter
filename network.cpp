/**************************************************************************
 *   network.cpp  --  This file is part of MKMCXX-NA.                     *
 *                                                                        *
 *   Copyright (C) 2017, Ivo Filot                                        *
 *                                                                        *
 *   MKMCXX-NA is free software:                                          *
 *   you can redistribute it and/or modify it under the terms of the      *
 *   GNU General Public License as published by the Free Software         *
 *   Foundation, either version 3 of the License, or (at your option)     *
 *   any later version.                                                   *
 *                                                                        *
 *   MKMCXX-NA is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty          *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *   See the GNU General Public License for more details.                 *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program.  If not, see http://www.gnu.org/licenses/.  *
 *                                                                        *
 **************************************************************************/

#include "network.h"

Node::Node(const std::string &_name) {
    this->name = _name;
    this->coverage = 0.0;
    this->highlight = false;
    this->selected = false;
    this->endpoint = false;
    this->disable_flag = false;
}

void Node::disable() {
    this->disable_flag = true;

    for(auto link : this->links) {
        link->disable();
    }
}

void Node::enable() {
    this->disable_flag = false;

    for(auto link : this->links) {
        link->enable();
    }
}

void Node::set_highlight() {
    if(this->highlight == false) {
        this->highlight = true;
        for(auto& link : this->links) {
            link->set_highlight();
        }
    }
}

void Node::disable_highlight() {
    if(this->highlight) {
        this->highlight = false;
        for(auto& link : this->links) {
            link->disable_highlight();
        }
    }
}

Link::Link(Node* _left, Node* _right, unsigned int _scl, unsigned int _scr) {
    this->left = _left;
    this->right = _right;
    this->scl = _scl;
    this->scr = _scr;
    this->flux = 0.0;
    this->disable_flag = false;
    this->highlight = false;
}

Network::Network() {
    // set variables
    this->insert_x = 0;
    this->drag_node = nullptr;

    // for random node placement
    srand (time(NULL));

    // load shaders and objects
    this->load_vao_lines();
}

void Network::draw() {
    this->draw_grid();
    this->draw_nodes();
    this->draw_links();
    this->draw_text();
}

void Network::draw_grid() {
    // load variables
    static const glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 0.1f);
    const glm::mat4 projection = Camera::get().get_projection();
    static const float z = 0.01f;

    this->shader_lines->link_shader();

    glBindVertexArray(this->vao_grid);
    this->shader_lines->set_uniform("color", &color);
    this->shader_lines->set_uniform("mvp", &projection);
    this->shader_lines->set_uniform("z", &z);
    glDrawElements(GL_LINES, this->nrindices_grid * 2, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    this->shader_lines->unlink_shader();
}

void Network::draw_nodes() {
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    const glm::mat4 projection = Camera::get().get_projection();
    const glm::mat4 scale = glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
    static const float z = 1.0f;

    // draw the nodes
    this->shader_nodes->link_shader();
    glBindVertexArray(this->vao_node);
    this->shader_nodes->set_uniform("color", &color);
    this->shader_nodes->set_uniform("z", &z);

    for(const auto &node : this->nodes) {

        if(node.second->is_disabled()) {
            continue;
        }

        if(node.second->is_endpoint()) {
            color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        } else {
            color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        }

        if(node.second->is_highlight()) {
            color = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
        }

        if(node.second->is_selected()) {
            color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        }

        const glm::mat4 mvp = projection * glm::translate(glm::vec3(node.second->get_x(), node.second->get_y(), 0.0f)) * scale;
        this->shader_nodes->set_uniform("mvp", &mvp);
        this->shader_lines->set_uniform("color", &color);
        glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
    this->shader_nodes->unlink_shader();

    // draw lines around nodes
    glm::vec4 color_lines = glm::vec4(0.0f,0.0f,0.0f,1.0f);

    this->shader_lines->link_shader();
    glBindVertexArray(this->vao_node);
    this->shader_lines->set_uniform("color", &color_lines);
    this->shader_lines->set_uniform("z", &z);

    for(const auto &node : this->nodes) {
        if(node.second->is_disabled()) {
            continue;
        }

        if(node.second->is_endpoint()) {
            color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        }

        const glm::mat4 mvp = projection * glm::translate(glm::vec3(node.second->get_x(), node.second->get_y(), 0.0f)) * scale;

        this->shader_lines->set_uniform("mvp", &mvp);
        this->shader_lines->set_uniform("color", &color);
        glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
    this->shader_lines->unlink_shader();
}

void Network::draw_links() {
    // update vao for the link lines
    this->update_vao_lines();

    // load variables
    glm::vec4 color = glm::vec4(1.0f,1.0f,1.0f,0.7f);
    const glm::mat4 projection = Camera::get().get_projection();
    const glm::mat4 scale = glm::scale(glm::vec3(0.2f, 0.2f, 0.2f));
    float z = 0.1f;
    static const int num_steps = 25;

    // draw the curves for the links
    this->shader_curves->link_shader();
    glBindVertexArray(this->vao_lines);
    this->shader_curves->set_uniform("mvp", &projection);
    this->shader_curves->set_uniform("z", &z);
    this->shader_curves->set_uniform("num_steps", &num_steps);

    for(unsigned int i=0; i<this->links.size(); i++) {
        if(links[i]->is_disabled()) {
            continue;
        }

        // set color
        if(links[i]->is_highlight()) {
            color = glm::vec4(1.0f,0.5f,0.0f,1.0f);
        } else {
            color = glm::vec4(1.0f,1.0f,1.0f,0.7f);
        }
        this->shader_curves->set_uniform("color", &color);

        glDrawArrays(GL_LINES_ADJACENCY_EXT, i*4, 4);
    }

    glBindVertexArray(0);
    this->shader_curves->unlink_shader();

    // draw the triangles for the links
    z = 1.0f;
    this->shader_nodes->link_shader();
    glBindVertexArray(this->vao_triangle);
    this->shader_nodes->set_uniform("z", &z);

    for(const auto &link : this->links) {
        if(link->is_disabled()) {
            continue;
        }

        const float cx = (link->get_node_left()->get_x() + link->get_node_right()->get_x()) / 2.0f;
        const float cy = (link->get_node_left()->get_y() + link->get_node_right()->get_y()) / 2.0f;

        // determine orientation of direction-triangle between nodes
        float angle;
        if(std::abs(link->get_node_left()->get_y() - link->get_node_right()->get_y()) < 1e-3) {
            if(link->get_node_left()->get_x() < link->get_node_right()->get_x()) {
                if(link->get_flux() > 0) {
                    angle = -M_PI / 2.0f;
                } else {
                    angle = M_PI / 2.0f;
                }
            } else {
                if(link->get_flux() > 0) {
                    angle = M_PI / 2.0f;
                } else {
                    angle = -M_PI / 2.0f;
                }
            }
        } else {
            if(link->get_node_left()->get_y() > link->get_node_right()->get_y()) {
                if(link->get_flux() > 0) {
                    angle = M_PI;
                } else {
                    angle = 0.0f;
                }
            } else {
                if(link->get_flux() > 0) {
                    angle = 0.0f;
                } else {
                    angle = M_PI;
                }
            }
        }

        if(link->is_highlight()) {
            color = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
        } else {
            color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        }

        const glm::mat4 trans = glm::translate(glm::vec3(cx, cy, 0.5f));
        const glm::mat4 rot = glm::rotate(angle, glm::vec3(0,0,1));

        const glm::mat4 mvp = projection * trans * rot * scale;
        this->shader_lines->set_uniform("color", &color);
        this->shader_lines->set_uniform("mvp", &mvp);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
    this->shader_nodes->unlink_shader();
}

void Network::draw_text() {
    // calculate inverse draw matrix
    const glm::mat4 projection = Camera::get().get_projection() * Camera::get().get_view();

    // draw node names
    for(const auto &node : this->nodes) {
        if(node.second->is_disabled()) {
            continue;
        }

        glm::vec4 pos = glm::vec4(node.second->get_x() - 0.25f, node.second->get_y() + 0.4f, 0.f, 1.0f);
        glm::vec4 screen_pos = projection * pos;

        float x = Screen::get().get_resolution_x() * (screen_pos[0] + 1.0f) / 2.0f;
        float y = Screen::get().get_resolution_y() * (screen_pos[1] + 1.0f) / 2.0f;

        FontWriter::get().write_text(1, x, y, 0.5f, glm::vec3(1,1,1), node.second->get_name());

        pos = glm::vec4(node.second->get_x() - 0.25f, node.second->get_y() - 0.7f, 0.f, 1.0f);
        screen_pos = projection * pos;

        x = Screen::get().get_resolution_x() * (screen_pos[0] + 1.0f) / 2.0f;
        y = Screen::get().get_resolution_y() * (screen_pos[1] + 1.0f) / 2.0f;

        FontWriter::get().write_text(1, x, y, 0.5f, glm::vec3(1,1,1), (boost::format("[%4.3f]") % node.second->get_coverage()).str());
    }

    for(const auto &link : this->links) {
        if(link->is_disabled()) {
            continue;
        }

        // calculate center point
        float cx = (link->get_node_left()->get_x() + link->get_node_right()->get_x()) / 2.0f;
        float cy = (link->get_node_left()->get_y() + link->get_node_right()->get_y()) / 2.0f;

        const glm::vec4 pos = glm::vec4(cx - 0.25f, cy + 0.4f, 0.f, 1.0f);
        const glm::vec4 screen_pos = projection * pos;

        float x = Screen::get().get_resolution_x() * (screen_pos[0] + 1.0f) / 2.0f;
        float y = Screen::get().get_resolution_y() * (screen_pos[1] + 1.0f) / 2.0f;

        glm::vec3 color;
        if(link->is_highlight()) {
            color = glm::vec3(1.0f, 0.5f, 0.0f);
        } else {
            color = glm::vec3(1.0f, 1.0f, 1.0f);
        }

        std::string flux_str = (boost::format("%4.2e") % std::abs(link->get_flux())).str();
        FontWriter::get().write_text(1, x, y, 0.9f, color, flux_str);
    }

    // draw network information
    FontWriter::get().write_text(0, 10.f, Screen::get().get_resolution_y() - 70.f, 0.9f, glm::vec3(1,1,1),
                                 (boost::format("Temperature: %0.0f") % this->temperature).str());
}

void Network::add_link(const Reaction& reaction, double flux) {
    this->add_link(reaction.get_reactants(),
                   reaction.get_products(),
                   reaction.get_reactant_coefficients(),
                   reaction.get_product_coefficients(),
                   flux);
}

/*
 * add links based on elementary reaction steps
 */
void Network::add_link(const std::vector<std::string> &lhs,
                       const std::vector<std::string> &rhs,
                       const std::vector<unsigned int> &lidx,
                       const std::vector<unsigned int> &ridx,
                       double flux) {

    // check that all items are already nodes
    for(const auto &s : lhs) {
        auto got = this->nodes.find(s);

        if(got == this->nodes.end()) { // insert node
            this->nodes.emplace(s, std::unique_ptr<Node>(new Node(s)) );
            got = this->nodes.find(s);
            if (s.find('*') == std::string::npos) {
                got->second->set_endpoint();
            }
            got->second->set_x((int)(rand() % 20) - 10);
            got->second->set_y((int)(rand() % 20) - 10);
            this->insert_x += 2.0f;
        }
    }

    for(const auto &s : rhs) {
        auto got = this->nodes.find(s);

        if(got == this->nodes.end()) { // insert node
            this->nodes.emplace(s, std::unique_ptr<Node>(new Node(s)) );
            got = this->nodes.find(s);
            if (s.find('*') == std::string::npos) {
                got->second->set_endpoint();
            }
            got->second->set_x((int)(rand() % 20) - 10);
            got->second->set_y((int)(rand() % 20) - 10);
            this->insert_x += 2.0f;
        }
    }

    // loop over all possible combinations and form links
    for(unsigned int i=0; i<lhs.size(); i++) {
        Node* left_node = this->nodes.find(lhs[i])->second.get();
        for(unsigned int j=0; j<rhs.size(); j++) {
            Node* right_node = this->nodes.find(rhs[j])->second.get();

            // check that no link already exists
            bool exists = false;
            for(auto& link : links) {
                // same node, same direction, so add the flux
                if(link->get_node_left() == left_node && link->get_node_right() == right_node) {
                    link->set_flux(link->get_flux() + flux);
                    exists = true;
                }

                // same node, but reverse direction, so reverse the flux
                if(link->get_node_right() == left_node && link->get_node_left() == right_node) {
                    link->set_flux(link->get_flux() - flux);
                    exists = true;
                }
            }

            // if no such links already exists, add a new one and add a pointer to the link in the nodes
            if(!exists) {
                this->links.emplace_back(new Link(left_node, right_node, lidx[i], ridx[j]));
                left_node->add_link(this->links.back().get());
                right_node->add_link(this->links.back().get());
                this->links.back()->set_flux(flux);
            }
        }
    }
}

void Network::set_coverage(const std::string& key, double coverage) {
    auto got = this->nodes.find(key);
    if(got != this->nodes.end()) {
        got->second->set_coverage(coverage);
    }
}


void Network::add_coverage(const std::string& key, double coverage) {
    auto got = this->nodes.find(key);
    if(got != this->nodes.end()) {
        got->second->add_coverage(coverage);
    }
}

void Network::handle_mouse(float x, float y) {
    if(this->drag_node != nullptr) {
        this->drag_node->set_x(std::round(x));
        this->drag_node->set_y(std::round(y));
        this->update_vao_lines();
        return;
    }

    static const float node_offset = 0.25f;

    Node* found = nullptr;
    for(auto& node : nodes) {
        if(node.second->is_disabled()) {
            continue;
        }

        const float cx = node.second->get_x();
        const float cy = node.second->get_y();

        if(x > cx - node_offset && x < cx + node_offset &&
           y > cy - node_offset && y < cy + node_offset) {
            found = node.second.get();
            break; // exit for loop
        }
    }

    for(auto& node : nodes) {
        node.second->disable_highlight();
    }

    if(found != nullptr) {
        found->set_highlight();
    }
}

void Network::left_click() {
    if(this->drag_node != nullptr) {
        this->drag_node->toggle_selected();
        this->drag_node = nullptr;
        return;
    }

    static const float node_offset = 0.25f;

    // recast mouse x,y positions to world positions
    const double xh = 2.0 * (Mouse::get().get_x() / (double)Screen::get().get_resolution_x() - 0.5);
    const double yh = 2.0 * ((1.0 - Mouse::get().get_y() / (double)Screen::get().get_resolution_y()) - 0.5);

    const glm::mat4 projection = Camera::get().get_projection();
    const glm::mat4 inv = glm::inverse(projection);

    const glm::vec4 wp = inv * glm::vec4(xh, yh, 0.0f, 1.0f);

    for(auto& node : nodes) {
        const float cx = node.second->get_x();
        const float cy = node.second->get_y();

        if(wp[0] > cx - node_offset && wp[0] < cx + node_offset &&
           wp[1] > cy - node_offset && wp[1] < cy + node_offset) {

            node.second->set_selected();
            this->drag_node = node.second.get();
            return;
        }
    }
}

void Network::reshuffle() {
    for(auto& node : nodes) {
        node.second->set_x((int)(rand() % 20) - 10);
        node.second->set_y((int)(rand() % 20) - 10);
    }
}

void Network::delete_node() {
    if(this->drag_node != nullptr) {
        this->drag_node->toggle_selected();
        this->drag_node->disable();
        this->drag_node = nullptr;
    }
}

void Network::load_vao_lines() {
    glGenVertexArrays(1, &this->vao_lines);
    glBindVertexArray(this->vao_lines);
    glGenBuffers(2, this->vbo_lines);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_lines[0]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->vbo_lines[1]);
    glBindVertexArray(0);
}

void Network::update_vao_lines() {
    std::vector<float> positions;
    std::vector<unsigned int> indices;

    unsigned int i = 0;
    for(const auto &link : this->links) {
        // left end point
        positions.push_back(link->get_node_left()->get_x());
        positions.push_back(link->get_node_left()->get_y());

        // left control point
        positions.push_back(link->get_node_right()->get_x());
        positions.push_back(link->get_node_left()->get_y());

        //right control point
        positions.push_back(link->get_node_left()->get_x());
        positions.push_back(link->get_node_right()->get_y());

        // right end point
        positions.push_back(link->get_node_right()->get_x());
        positions.push_back(link->get_node_right()->get_y());

        for(unsigned int j=0; j<4; j++) {
            indices.push_back(i+j);
        }

        i += 4;
    }

    glBindVertexArray(this->vao_lines);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_lines[0]);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), &positions[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->vbo_lines[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void Network::load_xml(const std::string& filename) {
    // create tree
    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(this->folder + "/" + filename, pt, boost::property_tree::xml_parser::trim_whitespace);

    // first enable all nodes
    for(const auto& node : this->nodes) {
        node.second->enable();
    }

    for(const auto& node : pt.get_child("network").get_child("nodes")) {
        std::string key = node.second.get<std::string>("name");
        float x = node.second.get<float>("x");
        float y = node.second.get<float>("y");
        bool disabled = node.second.get<bool>("disabled");

        auto got = this->nodes.find(key);
        if(got != this->nodes.end()) {
            got->second->set_x(x);
            got->second->set_y(y);
            if(disabled) {
                got->second->disable();
            }
        }
    }
}

void Network::save_xml(const std::string& filename) {
    // create tree
    boost::property_tree::ptree pt;

    for(const auto& node : this->nodes) {
        boost::property_tree::ptree child;
        child.put("name", node.second->get_name());
        child.put("disabled", node.second->is_disabled());
        child.put("x", node.second->get_x());
        child.put("y", node.second->get_y());

        pt.add_child("network.nodes.node", child);
    }

    boost::property_tree::xml_writer_settings<char> settings('\t', 1);
    boost::property_tree::write_xml(this->folder + "/" + filename, pt, std::locale(), settings);
}

Network::~Network() {
    // clean up vao and vbo
    glDeleteBuffers(2, this->vbo_lines);
    glDeleteVertexArrays(1, &this->vao_lines);
}
