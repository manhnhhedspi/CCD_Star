/*
 * polygon.cpp
 *
 *  Created on: Jun 23, 2015
 *      Author: sontd
 */

#include <boost/next_prior.hpp>
#include "../../include/common/segment.hpp"
#include "../../include/common/polygon.hpp"

namespace wandrian {
namespace common {

Polygon::Polygon() {
}

Polygon::Polygon(std::list<PointPtr> points) :
    points(points), graph() {
  std::list<PointPtr>::iterator point = this->points.begin();
  while (point != this->points.end()) {
    std::list<PointPtr>::iterator next =
        boost::next(point) != this->points.end() ?
            boost::next(point) : this->points.begin();
    if (*next == *point) {
      if (point != this->points.end())
        this->points.erase(next);
      else
        this->points.erase(point);
    } else
      point++;
  }
  build();
}

Polygon::~Polygon() {
  points.clear();
  for (std::map<PointPtr, std::set<PointPtr, PointComp> >::iterator n =
      graph.begin(); n != graph.end(); n++) {
    for (std::set<PointPtr>::iterator p = n->second.begin();
        p != (*n).second.end(); p++) {
      n->second.erase(p);
    }
    graph.erase(n);
  }
}

std::list<PointPtr> Polygon::get_points() {
  return points;
}

std::list<PointPtr> Polygon::get_boundary() {
  std::list<PointPtr> boundary;
  std::list<PointPtr> upper_boundary = get_upper_boundary();
  std::list<PointPtr> lower_boundary = get_lower_boundary();

  for (std::list<PointPtr>::iterator u = upper_boundary.begin();
      u != upper_boundary.end(); u++) {
    boundary.insert(boundary.end(), PointPtr(new Point(**u)));
  }

  for (std::list<PointPtr>::reverse_iterator l = boost::next(
      lower_boundary.rbegin()); boost::next(l) != lower_boundary.rend(); l++) {
    boundary.insert(boundary.end(), PointPtr(new Point(**l)));
  }
  return boundary;
}

void Polygon::build() {
  for (std::list<PointPtr>::iterator current = points.begin();
      current != points.end(); current++) {
    // Insert current point into graph if not yet
    if (graph.find(*current) == graph.end())
      graph.insert(
          std::pair<PointPtr, std::set<PointPtr, PointComp> >(*current,
              std::set<PointPtr, PointComp>()));
    // Find next point
    std::list<PointPtr>::iterator next;
    if (boost::next(current) != points.end())
      next = boost::next(current);
    else
      next = points.begin();
    // Insert next point into graph if not yet
    if (graph.find(*next) == graph.end())
      graph.insert(
          std::pair<PointPtr, std::set<PointPtr, PointComp> >(*next,
              std::set<PointPtr, PointComp>()));
    // Create edge
    if (current != next) {
      graph.find(*current)->second.insert(*next);
      graph.find(*next)->second.insert(*current);
    }
  }
  // Find all intersects
  std::map<SegmentPtr, std::set<PointPtr, PointComp>, SegmentComp> segments;
  for (std::map<PointPtr, std::set<PointPtr, PointComp> >::iterator current =
      graph.begin(); current != graph.end(); current++) {
    for (std::set<PointPtr>::iterator current_adjacent =
        current->second.begin(); current_adjacent != current->second.end();
        current_adjacent++) {
      for (std::map<PointPtr, std::set<PointPtr, PointComp> >::iterator another =
          boost::next(current); another != graph.end(); another++) {
        for (std::set<PointPtr>::iterator another_adjacent =
            another->second.begin(); another_adjacent != another->second.end();
            another_adjacent++) {
          SegmentPtr current_segment = SegmentPtr(
              new Segment(current->first, *current_adjacent));
          SegmentPtr another_segment = SegmentPtr(
              new Segment(another->first, *another_adjacent));
          PointPtr intersect = current_segment % another_segment;
          if (intersect) {
            if (segments.find(current_segment) == segments.end())
              segments.insert(
                  std::pair<SegmentPtr, std::set<PointPtr, PointComp> >(
                      current_segment, std::set<PointPtr, PointComp>()));
            if (segments.find(another_segment) == segments.end())
              segments.insert(
                  std::pair<SegmentPtr, std::set<PointPtr, PointComp> >(
                      another_segment, std::set<PointPtr, PointComp>()));

            if (intersect != current->first && intersect != *current_adjacent)
              segments.find(current_segment)->second.insert(intersect);
            if (intersect != another->first && intersect != *another_adjacent)
              segments.find(another_segment)->second.insert(intersect);
          }
        }
      }
    }
  }
  // Insert intersects and new edges into graph
  for (std::map<SegmentPtr, std::set<PointPtr, PointComp> >::iterator segment =
      segments.begin(); segment != segments.end(); segment++) {
    for (std::set<PointPtr>::iterator intersect = segment->second.begin();
        intersect != segment->second.end(); intersect++) {
      graph.find(segment->first->p1)->second.insert(*intersect);
      graph.find(segment->first->p2)->second.insert(*intersect);
      if (graph.find(*intersect) == graph.end())
        graph.insert(
            std::pair<PointPtr, std::set<PointPtr, PointComp> >(*intersect,
                std::set<PointPtr, PointComp>()));
      graph.find(*intersect)->second.insert(segment->first->p1);
      graph.find(*intersect)->second.insert(segment->first->p2);
      for (std::set<PointPtr>::iterator another = segment->second.begin();
          another != segment->second.end(); another++) {
        if (*intersect != *another)
          graph.find(*intersect)->second.insert(*another);
      }
    }
  }
  // TODO: Remove redundant edges
}

PointPtr Polygon::get_leftmost() {
  PointPtr leftmost = *(points.begin());
  for (std::list<PointPtr>::iterator current = boost::next(points.begin());
      current != points.end(); current++) {
    if (*current < leftmost)
      leftmost = *current;
  }
  return leftmost;
}

PointPtr Polygon::get_rightmost() {
  PointPtr rightmost = *(points.begin());
  for (std::list<PointPtr>::iterator current = boost::next(points.begin());
      current != points.end(); current++) {
    if (*current > rightmost)
      rightmost = *current;
  }
  return rightmost;
}

std::list<PointPtr> Polygon::get_upper_boundary() {
  return get_partial_boundary(true);
}

std::list<PointPtr> Polygon::get_lower_boundary() {
  return get_partial_boundary(false);
}

std::list<PointPtr> Polygon::get_partial_boundary(bool is_upper) {
  std::list<PointPtr> partial_boundary;
  PointPtr leftmost = get_leftmost();
  PointPtr rightmost = get_rightmost();
  partial_boundary.insert(partial_boundary.end(), leftmost);

  PointPtr current = leftmost;
  PointPtr previous = PointPtr(new Point(current->x - 1, current->y));
  while (current != rightmost) {
    double angle;
    double distance = std::numeric_limits<double>::infinity();
    if (is_upper)
      angle = 2 * M_PI;
    else
      angle = 0;
    PointPtr next;
    for (std::set<PointPtr>::iterator adjacent =
        graph.find(current)->second.begin();
        adjacent != graph.find(current)->second.end(); adjacent++) {
      double a = std::atan2(previous->y - current->y, previous->x - current->x)
          - std::atan2((*adjacent)->y - current->y,
              (*adjacent)->x - current->x);
      double d = std::sqrt(
          std::pow(current->x - (*adjacent)->x, 2)
              + std::pow(current->y - (*adjacent)->y, 2));
      if (is_upper) {
        a = std::abs(a) <= EPSILON ? 2 * M_PI : a > 0 ? a : 2 * M_PI + a;
        if (a - angle < -EPSILON
            || (std::abs(a - angle) < EPSILON && d < distance)) {
          angle = a;
          distance = d;
          next = *adjacent;
        }
      } else {
        a = std::abs(a) <= EPSILON ? 0 : a > 0 ? a : 2 * M_PI + a;
        if (a - angle > EPSILON
            || (std::abs(a - angle) < EPSILON && d < distance)) {
          angle = a;
          distance = d;
          next = *adjacent;
        }
      }
    }
    previous = current;
    current = next;
    partial_boundary.insert(partial_boundary.end(), current);
  }
  return partial_boundary;
}

}
}
