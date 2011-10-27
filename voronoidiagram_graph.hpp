/*  
 *  Copyright 2010-2011 Anders Wallin (anders.e.e.wallin "at" gmail.com)
 *  
 *  This file is part of OpenVoronoi.
 *
 *  OpenCAMlib is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCAMlib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with OpenCAMlib.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef VODI_G_HPP
#define VODI_G_HPP

#include <vector>

#include <boost/graph/adjacency_list.hpp>

#include "point.hpp"
#include "halfedgediagram.hpp"
#include "voronoivertex.hpp"

// this file contains typedefs used by voronoidiagram.h
namespace ovd {

// notes from the Okabe-Boots-Sugihara book, page 171->:
/* 
 * Distance-function.
 * R1 - region of endpoint pi1
 * R2 - region of endpoint pi2
 * R3 - region of line-segment Li
 *               dist(p,pi1) if  p in R1
 * dist(p,Li) =  dist(p,pi2) if  p in R2
 *               dist(p,Li)  if p in R3
 * 
 * dist(p,Li) = distance from p to L, along perpendicular line to L
 * 
 * = norm(  (x-xi1)   -  dotprod( (x-xi1), (xi2-xi1) ) / ( norm_sq(xi2-xi1) ) * (xi2,xi1) )
 * 
 * 
 * 
 * Vertex - LineSegment
 * Bisectors:
 *  B1: point-point: line
 *  B2: point-line: parabola
 *  B3: line-line: line
 * 
 *  Voronoi Edges:
 *  E1: point pi - point pj. straight line bisecting pi-pj
 *  E2: edge generated by line-segment L's endpoint pi. perpendicular to L, passing through pi
 *  E3: point pi - segment Lj. dist(E3, p) == dist(E3,Lj). Parabolic arc
 *  E4: line Li - Line Lj. straight line bisector
 *  (G): generator segment edge
 * 
 *  Voronoi vertices (see p177 of Okabe book):
 *  V1: generators(pi, pj, pk). edges(E1, E1, E1)
 *     - compute using detH. This is also the circumcenter of the pi,pj,pk triangle
 *  V2: generators(pi, Lj, pj1) point, segment, segment's endpoint. edges(E1, E2, E3)   E1 and E3 are tangent at V2
 *     - ? compute by mirroring pi in the separator and use detH
 *  V3: generators(Li, pj, pk) edges(E1, E3, E3)   E3-edges have common directrix(Li)
 *  V4: generators(Li, Lj, pi1)  edges(E2, E3, E4)  E3-E4 tangent at V4
 *  V5: generators(pi, Lj, Lk) edges (E3, E3, E4)
 *  V6: generators(Li, Lj, Lk) edges(E4, E4, E4)
 *    - this is the incenter of a incircle inscribed by triangle Li,Lj,Lk (or sometiems excenter of excircle if V6 outside triangle?)
 *    - The Cartesian coordinates of the incenter are a weighted average of the coordinates of the three vertices using the side 
 *       lengths of the triangle as weights. (The weights are positive so the incenter lies inside the triangle as stated above.) 
 *      If the three vertices are located at (xa,ya), (xb,yb), and (xc,yc), and the sides opposite these vertices have corresponding 
 *      lengths a, b, and c, then the incenter is at   
 *      (a x_a + b x_b + c x_c)/ (a+b+c) 
 * 
 * bisector formulas
 * x = x1 - x2 - x3*t +/- x4 * sqrt( square(x5+x6*t) - square(x7+x8*t) )
 * (same formula for y-coordinate)
 * line (line/line)
 * parabola (circle/line)
 * hyperbola (circle/circle)
 * ellipse (circle/circle)
 * 
 * line: a1*x + b1*y + c + k*t = 0  (t is amount of offset) k=+1 offset left of line, k=-1 right of line
 * with a*a + b*b = 1
 * 
 * circle: square(x-xc) + square(y-yc) = square(r+k*t)  k=+1 for enlarging circle, k=-1 shrinking
 */
 

// use traits-class here so that EdgePros can store data of type HEEdge
// typedef of the VD-graph follows below. 

// vecS is slightly faster than listS
// vecS   5.72us * n log(n)
// listS  6.18 * n log(n)
#define OUT_EDGE_CONTAINER boost::vecS 

// note: cannot use vecS since remove_vertex invalidates iterators/edge_descriptors (?)
#define VERTEX_CONTAINER boost::listS
#define EDGE_LIST_CONTAINER boost::listS

// type of edge-descriptors in the graph
typedef boost::adjacency_list_traits<OUT_EDGE_CONTAINER, 
                                     VERTEX_CONTAINER, 
                                     boost::bidirectionalS, 
                                     EDGE_LIST_CONTAINER >::edge_descriptor HEEdge;
// type of face-descriptors in the graph
typedef unsigned int HEFace;    

/*
 * bisector formulas
 * x = x1 - x2 - x3*t +/- x4 * sqrt( square(x5+x6*t) - square(x7+x8*t) )
 * (same formula for y-coordinate)
 * line (line/line)
 * parabola (circle/line)
 * hyperbola (circle/circle)
 * ellipse (circle/circle)
*/
                            
enum VoronoiEdgeType {LINE, PARABOLA, ELLIPSE, HYPERBOLA, SEPARATOR, LINESITE};

/// properties of an edge in the voronoi diagram
/// each edge stores a pointer to the next HEEdge 
/// and the HEFace to which this HEEdge belongs
struct EdgeProps {
    EdgeProps() {}
    EdgeProps(HEEdge n, HEFace f): next(n), face(f) {}
    /// create edge with given next, twin, and face
    EdgeProps(HEEdge n, HEEdge t, HEFace f): next(n), twin(t), face(f) {}
    /// the next edge, counterclockwise, from this edge
    HEEdge next; 
    /// the twin edge
    HEEdge twin;
    /// the face to which this edge belongs
    HEFace face; // each face corresponds to an input Site/generator
    double k; // offset-direction from the adjacent site, either +1 or -1
    VoronoiEdgeType type;
    inline double sq(double x) const {return x*x;}
    Point point(double t) const {
        double discr1 = sq(x[4]+x[5]*t) - sq(x[6]+x[7]*t);
        double discr2 = sq(y[4]+y[5]*t) - sq(y[6]+y[7]*t);
        if ( (discr1 >= 0) && (discr2 >= 0) ) {
            double xc = x[0] - x[1] - x[2]*t + x[3] * sqrt( sq(x[4]+x[5]*t) - sq(x[6]+x[7]*t) );
            double yc = y[0] - y[1] - y[2]*t + y[3] * sqrt( sq(y[4]+y[5]*t) - sq(y[6]+y[7]*t) );
            return Point(xc,yc);
        } else {
            std::cout << " warning bisector sqrt(-1) !\n";
            return Point(0,0);
        }
    }
    double x[8];
    double y[8];
    void set_parameters(Site* s1, Site* s2) {
        if (s1->isPoint() && s2->isPoint())        // PP
            set_pp_parameters(s1,s2);
        else if (s1->isPoint() && s2->isLine())    // PL
            set_pl_parameters(s1,s2);
        else if (s2->isPoint() && s1->isLine())    // LP
            set_pl_parameters(s2,s1);
        else if (s1->isLine() && s2->isLine())     // LL
            set_ll_parameters(s2,s1);
        else
            assert(0);
            // AP
            // PA
            // AA
            // AL
            // LA
    }
    // called for point(s1)-point(s2) edges
    void set_pp_parameters(Site* s1, Site* s2) {
        std::cout << " set_pp \n";
        type = LINE;
    }
    // called for point(s1)-line(s2) edges
    void set_pl_parameters(Site* s1, Site* s2) {
        std::cout << " set_pl \n";
        type = PARABOLA;
        double alfa3 = s2->a()*s1->x() + s2->b()*s1->y() + s2->c();
        x[0]=s1->x();       // xc1
        x[1]=s2->a()*alfa3; // alfa1*alfa3
        x[2]=-s2->a();      // -alfa1 = - a2
        x[3]=s2->b();       // alfa2 = b2
        x[4]=0;             // alfa4 = r1 
        x[5]=+1;            // lambda1
        x[6]= alfa3;        // alfa3= a2*xc1+b2*yc1+d2?
        x[7]=-1;            // -1 

        y[0]=s1->y();       // yc1
        y[1]=s2->b()*alfa3; // alfa2*alfa3
        y[2]=-s2->b();      // -alfa2 = -b2
        y[3]=s2->a();       // alfa1 = a2
        y[4]=0;             // alfa4 = r1
        y[5]=+1;            // lambda1
        y[6]=alfa3;         // alfa3
        y[7]=-1;            // -1
        print_params();
    }
    // line(s1)-line(s2) edge
    void set_ll_parameters(Site* s1, Site* s2) {  // Held thesis p96
        std::cout << " set_ll \n";
        type = LINE;
        double delta = s1->a()*s2->b() - s1->b()*s2->a();
        assert( delta != 0 );
        x[0]= ( (s1->b() * s2->c()) - (s2->b() * s1->c()) ) / delta;  // alfa1 = (b1*d2-b2*d1) / delta
        x[1]=0;
        x[2]= -(s2->b()-s1->b()); // -alfa3 = -( b2-b1 )
        x[3]=0;
        x[4]=0;
        x[5]=0;
        x[6]=0;
        x[7]=0;
        
        y[0]= ( (s2->a()*s1->c()) - (s1->a()*s2->c()) ) / delta;  // alfa2 = (a2*d1-a1*d2) / delta
        y[1]= 0;
        y[2]= -(s1->a()-s2->a());  // -alfa4 = -( a1-a2 )
        y[3]=0;
        y[4]=0;
        y[5]=0;
        y[6]=0;
        y[7]=0;
        y[8]=0;
    }
    void print_params() const {
        std::cout << "x-params: ";
        for (int m=0;m<8;m++)
            std::cout << x[m] << " ";
        std::cout << "\n";
        std::cout << "y-params: ";
        for (int m=0;m<8;m++)
            std::cout << y[m] << " ";
        std::cout << "\n";
    }
    // arc: d=sqrt( sq(xc1-xc2) + sq(yc1-yc2) )
};


/// Status of faces in the voronoi diagram
/// INCIDENT faces contain one or more IN-vertex
/// NONINCIDENT faces contain only OUT-vertices
enum VoronoiFaceStatus {INCIDENT, NONINCIDENT};

/// properties of a face in the voronoi diagram
/// each face stores one edge on the boundary of the face
struct FaceProps {
    FaceProps( ) {}
    /// create face with given edge, generator, and type
    FaceProps( HEEdge e , Site* s, VoronoiFaceStatus st) : edge(e), site(s), status(st) {}
    /// operator for sorting faces
    bool operator<(const FaceProps& f) const {return (this->idx<f.idx);}
    /// face index
    HEFace idx;
    /// one edge that bounds this face
    HEEdge edge;
    /// the site/generator for this face (either PointSite, LineSite, or ArcSite)
    Site* site;
    /// face status (either incident or nonincident)
    VoronoiFaceStatus status;
};

// the type of graph with which we construct the voronoi-diagram
typedef hedi::HEDIGraph< OUT_EDGE_CONTAINER,     // out-edges stored in a std::list
                       VERTEX_CONTAINER,         // vertex set stored here
                       boost::bidirectionalS,    // bidirectional graph.
                       VoronoiVertex,            // vertex properties
                       EdgeProps,                // edge properties
                       FaceProps,                // face properties
                       boost::no_property,       // graph properties
                       EDGE_LIST_CONTAINER       // edge storage
                       > HEGraph;
// NOTE: if these listS etc. arguments ever change, they must also be updated
// above where we do: adjacency_list_traits

typedef boost::graph_traits< HEGraph::BGLGraph >::vertex_descriptor  HEVertex;
typedef boost::graph_traits< HEGraph::BGLGraph >::vertex_iterator    HEVertexItr;
typedef boost::graph_traits< HEGraph::BGLGraph >::edge_iterator      HEEdgeItr;
typedef boost::graph_traits< HEGraph::BGLGraph >::out_edge_iterator  HEOutEdgeItr;
typedef boost::graph_traits< HEGraph::BGLGraph >::adjacency_iterator HEAdjacencyItr;
typedef boost::graph_traits< HEGraph::BGLGraph >::vertices_size_type HEVertexSize;

// these containers are used instead of iterators when accessing
// adjacent vertices, edges, faces.
// FIXME: it may be faster to rewrite the code so it uses iterators, as does the BGL.
typedef std::vector<HEVertex> VertexVector;
typedef std::vector<HEFace> FaceVector;
typedef std::vector<HEEdge> EdgeVector;  

} // end namespace
#endif


