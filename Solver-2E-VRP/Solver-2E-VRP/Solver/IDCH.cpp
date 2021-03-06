//
// Created by Youcef on 14/03/2016.
//

#include "IDCH.h"
#include "../Utility.h"
#include "./SDVRPSolver.h"
#include <algorithm>

/****************************************************
 *  
 *                IDCH HEURISTICS
 *  
 ****************************************************/
void IDCH::heuristicIDCH(Solution &bestSolution) {
    int n = this->problem->getClients().size(),
            iter = 0,
            itermax = n * n;

    if (bestSolution.getTotalCost() == 0) {
        this->doGreedyInsertion(bestSolution);
    }

    Solution solution = bestSolution;

    while (iter < itermax) {
        // Large destruction step
        if ((iter + 1) % n == 0) {
            this->doDestroy(solution);
        }
            // Small destruction step
        else {
            doDestroySmall(solution);
        }
        // Destruction de la solution
        this->doDestroy(solution);

        // Perturbation de la solution
        this->apply2OptOnEachTour(solution);

        // Réparation de la solution
        this->doRepair(solution);

        if (solution.getTotalCost() < bestSolution.getTotalCost()) {
            bestSolution = solution;
            iter = 0;
        } else iter++;
    }
}

void IDCH::heuristicFastIDCH(Solution &bestSolution) {
    int n = this->problem->getClients().size(),
            iter = 0,
            itermax = n;

    if (bestSolution.getTotalCost() == 0) {
        this->doGreedyInsertion(bestSolution);
    }

    Solution solution = bestSolution;

    while (iter < itermax) {
        // Destruction de la solution
        this->doDestroySmall(solution);

        // Perturbation de la solution
        this->apply2OptOnEachTour(solution);

        // Réparation de la solution
        this->doRepair(solution);

        if (solution.getTotalCost() < bestSolution.getTotalCost()) {
            bestSolution = solution;
            iter = 0;
        } else iter++;
    }
}

/****************************************************
 *  
 *                REPAIR OPERATORS
 *  
 ****************************************************/
void IDCH::doGreedyInsertion(Solution &solution) {
    this->insertion.GreedyInsertionHeuristic(solution);
}

void IDCH::doGreedyInsertionPerturbation(Solution &solution) {

}

void IDCH::doMoleAndJamesonInsertion(Solution &solution) {

}

/****************************************************
 *  
 *                DESTROY OPERATORS
 *  
 ****************************************************/
void IDCH::doRandomRemoval(Solution &solution, double p1) {
    int n = problem->numberOfClients();
    // nc : number of customers to remove
    int nc = static_cast<int>(lround(p1 * n));
    n -= solution.unroutedCustomers.size();
    nc = min(nc, n);
    int rt, // Route number
            rc;  // Customer customer

    double cost;

    int i = 0;
    while (i < nc) {
        // Find a customer to remove
        rt = Utility::randomInt(0, solution.getE2Routes().size());
        rc = Utility::randomInt(0, solution.getE2Routes()[rt].tour.size());
        // Compute removal cost
        cost = removalCost(solution, rc, rt);
        // Insert customer into list of unrouted customers
        solution.unroutedCustomers.push_back(solution.getE2Routes()[rt].tour[rc]);

        // Remove route if it contains only one customer
        if (solution.getE2Routes()[rt].tour.size() == 1) {
            // Update satellite demand
            solution.getSatelliteDemands()[solution.getE2Routes()[rt].departureSatellite] -= problem->getClient(
                    solution.getE2Routes()[rt].tour[rc]).getDemand();
            // remove route
            solution.getE2Routes().erase(solution.getE2Routes().begin() + rt);
        }
        else {
            // Update route cost
            solution.getE2Routes()[rt].cost += cost;
            // Update route load
            solution.getE2Routes()[rt].load -= problem->getClient(solution.getE2Routes()[rt].tour[rc]).getDemand();
            // Update satellite demand
            solution.getSatelliteDemands()[solution.getE2Routes()[rt].departureSatellite] -= problem->getClient(
                    solution.getE2Routes()[rt].tour[rc]).getDemand();
            // Remove the customer from the route it is in
            solution.getE2Routes()[rt].tour.erase(solution.getE2Routes()[rt].tour.begin() + rc);
        }
        // Update solution cost
        solution.setTotalCost(solution.getTotalCost() + cost);
        // Go to the next customer
        i++;
    }
}

// Todo normalize distances
void IDCH::doWorstRemoval(Solution &solution, double p2) {

    int n = problem->numberOfClients();
    // nc : number of customers to remove
    int nc = static_cast<int>(lround(p2 * n));
    n -= solution.unroutedCustomers.size();
    nc = min(nc, n);
    int rt, // Route number
            rc;  // Customer customer

    double cost, tmpCost;

    int i = 0;
    while (i < nc) {
        cost = 0;
        rt = 0;
        rc = 0;
        // Find a customer to remove
        for (int u = 0; u < solution.getE2Routes().size(); ++u) {
            E2Route &route = solution.getE2Routes()[u];
            for (int v = 0; v < route.tour.size(); ++v) {
                tmpCost = removalCost(solution, v, u);
                if (tmpCost > cost) {
                    rt = u;
                    rc = v;
                    cost = tmpCost;
                }

            }
        }
        // Insert customer into list of unrouted customers
        solution.unroutedCustomers.push_back(solution.getE2Routes()[rt].tour[rc]);

        // Remove route if it contains only one customer
        if (solution.getE2Routes()[rt].tour.size() == 1) {
            // Update satellite demand
            solution.getSatelliteDemands()[solution.getE2Routes()[rt].departureSatellite] -= problem->getClient(
                    solution.getE2Routes()[rt].tour[rc]).getDemand();
            // remove route
            solution.getE2Routes().erase(solution.getE2Routes().begin() + rt);
        }
        else {
            // Update route cost
            solution.getE2Routes()[rt].cost += cost;
            // Update route load
            solution.getE2Routes()[rt].load -= problem->getClient(solution.getE2Routes()[rt].tour[rc]).getDemand();
            // Update satellite demand
            solution.getSatelliteDemands()[solution.getE2Routes()[rt].departureSatellite] -= problem->getClient(
                    solution.getE2Routes()[rt].tour[rc]).getDemand();
            // Remove the customer from the route it is in
            solution.getE2Routes()[rt].tour.erase(solution.getE2Routes()[rt].tour.begin() + rc);
        }
        // Update solution cost
        solution.setTotalCost(solution.getTotalCost() + cost);
        // Go to the next customer
        i++;
    }

}

struct Neighbor {
    int id;
    int rt;
    int rc;
    double distance;
};

bool neighborSort(Neighbor x, Neighbor y) { return x.distance < y.distance; }

void IDCH::doRelatedRemoval(Solution &solution, double p3) {
    int n = problem->numberOfClients();
    // nc : number of customers to remove
    int nc = static_cast<int>(lround(p3 * n));
    n -= solution.unroutedCustomers.size();
    nc = min(nc, n);
    int rt, // Route number
            rc;  // Customer customer

    double cost;

    // Chose a seed customer
    int seedRt = Utility::randomInt(0, solution.getE2Routes().size());
    int seedPos = Utility::randomInt(0, solution.getE2Routes()[seedRt].tour.size());
    Client seed = problem->getClient(solution.getE2Routes()[seedRt].tour[seedPos]);

    // Construct the list L containing the neighbors of seed
    vector<Neighbor> L;
    L.push_back(Neighbor{seed.getClientId(), seedRt, seedPos, 0});
    for (int j = 0; j < solution.getE2Routes().size(); ++j) {
        for (int k = 0; k < solution.getE2Routes()[j].tour.size(); ++k) {
            int id = solution.getE2Routes()[j].tour[k];
            if (id == seed.getClientId()) continue;
            L.push_back(Neighbor{id, j, k, problem->getDistance(seed, problem->getClients()[id])});
        }
    }
    // Sort L by increasing distance to seed
    std::sort(L.begin() + 1, L.end(), neighborSort);

    int i = 0;
    while (i < nc) {
        // Find a customer to remove
        rt = L[i].rt;
        rc = L[i].rc;
        // Compute removal cost
        cost = removalCost(solution, rc, rt);
        // Insert customer into list of unrouted customers
        solution.unroutedCustomers.push_back(L[i].id);

        // Remove route if it contains only one customer
        if (solution.getE2Routes()[rt].tour.size() == 1) {
            // Update satellite demand
            solution.getSatelliteDemands()[solution.getE2Routes()[rt].departureSatellite] -= problem->getClient(
                    L[i].id).getDemand();
            // remove route
            solution.getE2Routes().erase(solution.getE2Routes().begin() + rt);
            // Update value of rt in L entries
            for (int j = i + 1; j < nc; ++j) {
                if (L[j].rt > rt) L[j].rt--;
            }
        }
        else {
            // Update route cost
            solution.getE2Routes()[rt].cost += cost;
            // Update route load
            solution.getE2Routes()[rt].load -= problem->getClient(L[i].id).getDemand();
            // Update satellite demand
            solution.getSatelliteDemands()[solution.getE2Routes()[rt].departureSatellite] -= problem->getClient(
                    L[i].id).getDemand();
            // Remove the customer from the route it is in
            solution.getE2Routes()[rt].tour.erase(solution.getE2Routes()[rt].tour.begin() + rc);
            // Update value of rc in L entries
            for (int j = i + 1; j < nc; ++j) {
                if (L[j].rt == rt && L[j].rc > rc) L[j].rc--;
            }
        }
        // Update solution cost
        solution.setTotalCost(solution.getTotalCost() + cost);
        // Go to the next customer
        i++;
    }
}

void IDCH::doRouteRemoval(Solution &solution, int p4) {
    vector<E2Route> &routes = solution.getE2Routes();
    int size = routes.size();
    // nr : number of routes to remove
    int nr = Utility::randomInt(0, 1 + min(p4, size));
    // index of the route to be removed
    int r;
    while (nr > 0 && routes.size() > 0) {
        r = Utility::randomInt(0, routes.size());
        // Update solution cost
        solution.setTotalCost(solution.getTotalCost() - routes[r].cost);
        // Update satellite demand
        solution.getSatelliteDemands()[routes[r].departureSatellite] -= routes[r].load;
        // Insert customers into pool
        for (int i = 0; i < routes[r].tour.size(); ++i) {
            solution.unroutedCustomers.push_back(routes[r].tour[i]);
        }
        // Remove route
        routes.erase(routes.begin() + r);
        nr--;
    }

}

void IDCH::doRemoveSingleNodeRoutes(Solution &solution) {
    int i = 0;
    while (i < solution.getE2Routes().size()) {
        E2Route &route = solution.getE2Routes()[i];
        // If it's a single customer trip
        if (route.tour.size() == 1) {
            // Insert customer into the unrouted customer's pool
            solution.unroutedCustomers.push_back(route.tour[0]);
            // Update satellite demand
            solution.getSatelliteDemands()[route.departureSatellite] -= route.load;
            // Update solution cost
            solution.setTotalCost(solution.getTotalCost() - route.cost);
            // remove route
            solution.getE2Routes().erase(solution.getE2Routes().begin() + i);
        } else i++;
    }
}

void IDCH::doSatelliteRemoval(Solution &solution, double p6) {
    // Chose the satellite to close
    int sat;
    do {
        sat = Utility::randomInt(0, solution.satelliteState.size());
    } while (solution.satelliteState[sat] == Solution::CLOSED);
    // Remove all routes originating from the chosen satellite
    vector<E2Route> &routes = solution.getE2Routes();
    int r = 0;
    while (r < routes.size()) {
        // If the route originates from sat, then remove it
        if (routes[r].departureSatellite == sat) {
            // Update solution cost
            solution.setTotalCost(solution.getTotalCost() - routes[r].cost);
            // Insert customers into pool
            for (int i = 0; i < routes[r].tour.size(); ++i) {
                solution.unroutedCustomers.push_back(routes[r].tour[i]);
            }
            // Remove route
            routes.erase(routes.begin() + r);
        }
            // Else ignore it
        else r++;
    }
    // Update satellite demand
    solution.getSatelliteDemands()[sat] = 0;
    // Close satellite
    solution.satelliteState[sat] = Solution::CLOSED;
    solution.openSatellites--;
    // Open another satellite if they are all closed
    if (solution.openSatellites == 0) {
        sat = Utility::randomInt(0, solution.satelliteState.size());
        solution.satelliteState[sat] = Solution::OPEN;
        solution.openSatellites = 1;
    }

}

void IDCH::doSatelliteOpening(Solution &solution, double p7) {
    int n = problem->numberOfClients();
    // nc : number of customers to remove
    int nc = static_cast<int>(lround(p7 * n));
    n -= solution.unroutedCustomers.size();
    nc = min(nc, n);

    int rt, // Route number
            rc;  // Customer customer

    double cost;

    // Chose a satellite to open
    int satId;
    do {
        satId = Utility::randomInt(0, solution.satelliteState.size());
    } while (solution.satelliteState[satId] == Solution::OPEN);

    Satellite sat = problem->getSatellite(satId);

    // Open the satellite
    solution.satelliteState[satId] = Solution::OPEN;
    solution.openSatellites++;
    // Construct the list L containing the neighbors of sat
    vector<Neighbor> L;
    for (int j = 0; j < solution.getE2Routes().size(); ++j) {
        for (int k = 0; k < solution.getE2Routes()[j].tour.size(); ++k) {
            int id = solution.getE2Routes()[j].tour[k];
            L.push_back(Neighbor{id, j, k, problem->getDistance(sat, problem->getClients()[id])});
        }
    }
    // Sort L by increasing distance to sat
    std::sort(L.begin(), L.end(), neighborSort);

    // Remove the nc customers closest to sat
    int i = 0;
    while (i < nc && i < n) {
        // Find a customer to remove
        rt = L[i].rt;
        rc = L[i].rc;
        // Compute removal cost
        cost = removalCost(solution, rc, rt);
        // Insert customer into list of unrouted customers
        solution.unroutedCustomers.push_back(L[i].id);

        // Remove route if it contains only one customer
        if (solution.getE2Routes()[rt].tour.size() == 1) {
            // Update satellite demand
            solution.getSatelliteDemands()[solution.getE2Routes()[rt].departureSatellite] -= problem->getClient(
                    L[i].id).getDemand();
            // remove route
            solution.getE2Routes().erase(solution.getE2Routes().begin() + rt);
            // Update value of rt in L entries
            for (int j = i + 1; j < nc; ++j) {
                if (L[j].rt > rt) L[j].rt--;
            }
        }
        else {
            // Update route cost
            solution.getE2Routes()[rt].cost += cost;
            // Update route load
            solution.getE2Routes()[rt].load -= problem->getClient(L[i].id).getDemand();
            // Update satellite demand
            solution.getSatelliteDemands()[solution.getE2Routes()[rt].departureSatellite] -= problem->getClient(
                    L[i].id).getDemand();
            // Remove the customer from the route it is in
            solution.getE2Routes()[rt].tour.erase(solution.getE2Routes()[rt].tour.begin() + rc);
            // Update value of rc in L entries
            for (int j = i + 1; j < nc; ++j) {
                if (L[j].rt == rt && L[j].rc > rc) L[j].rc--;
            }
        }
        // Update solution cost
        solution.setTotalCost(solution.getTotalCost() + cost);
        // Go to the next customer
        i++;
    }
}

void IDCH::doOpenAllSatellites(Solution &solution) {
    std::fill(solution.satelliteState.begin(), solution.satelliteState.end(), Solution::OPEN);
    solution.openSatellites = solution.satelliteState.size();
}

/****************************************************
 *  
 *           PERTURBATION AND LOCAL SEARCH
 *  
 ****************************************************/
bool IDCH::apply2OptOnEachTour(Solution &solution) {
    bool improvement = false;
    double imp = 0;
    for (E2Route route : solution.getE2Routes()) {
        imp -= route.cost;
        improvement = improvement || lsSolver.apply2OptOnTour(route);
        imp += route.cost;
    }
    if (imp < -0.0001) solution.setTotalCost(solution.getTotalCost() + imp);
    return improvement;
}

/****************************************************
 *
 *        Destroy and Repair for IDCH
 *
 ****************************************************/
void IDCH::doDestroy(Solution &solution) {
    /* Todo 1 changer les paramètres
     * Todo 2 implémenter un schéma pour l'utilisation des opérateurs
    */
    this->doRandomRemoval(solution, 0.15);
    this->doWorstRemoval(solution, 0.25);
    this->doRelatedRemoval(solution, 0.1);
    //this->doRemoveSingleNodeRoutes(solution);
    //this->doRouteRemoval(solution, 5);
    this->doSatelliteRemoval(solution, 0);
    this->doSatelliteOpening(solution, 0.1);
    //this->doOpenAllSatellites(solution);
    /******************************/
    /*int choice = Utility::randomInt(0, 4);
    switch (choice) {
        case 0:
            this->doRandomRemoval(solution, 0.3);
            break;
        case 1:
            this->doWorstRemoval(solution, 0.4);
            break;
        case 2:
            this->doRouteRemoval(solution, 5);
            break;
        case 3:
            this->doRemoveSingleNodeRoutes(solution);
            break;
        default:
            this->doRandomRemoval(solution, 0.4);
            break;
    }*/
}

void IDCH::doDestroySmall(Solution &solution) {
/* Todo 1 changer les paramètres
     * Todo 2 implémenter un schéma pour l'utilisation des opérateurs
    */
    this->doRandomRemoval(solution, 0.1);
    this->doWorstRemoval(solution, 0.15);
    this->doRelatedRemoval(solution, 0.1);
    //this->doRemoveSingleNodeRoutes(solution);
    //this->doRouteRemoval(solution, 5);
    //this->doSatelliteRemoval(solution, 0);
    //this->doSatelliteOpening(solution, 0.1);
    //this->doOpenAllSatellites(solution);
    /******************************/
    /*int choice = Utility::randomInt(0, 4);
    switch (choice) {
        case 0:
            this->doRandomRemoval(solution, 0.3);
            break;
        case 1:
            this->doWorstRemoval(solution, 0.4);
            break;
        case 2:
            this->doRouteRemoval(solution, 5);
            break;
        case 3:
            this->doRemoveSingleNodeRoutes(solution);
            break;
        default:
            this->doRandomRemoval(solution, 0.4);
            break;
    }*/
}

// Todo : implement a repair method
void IDCH::doRepair(Solution &solution) {
    this->doGreedyInsertion(solution);
}

/****************************************************
 *  
 *                OTHER METHODS
 *  
 ****************************************************/
double IDCH::removalCost(Solution &solution, int customer, int route) {

    if (solution.getE2Routes()[route].tour.size() == 1)
        return 2 * problem->getDistance(problem->getSatellite(solution.getE2Routes()[route].departureSatellite),
                                        problem->getClient(solution.getE2Routes()[route].tour[customer]));

    Node p, q, c;
    c = problem->getClient(solution.getE2Routes()[route].tour[customer]);
    if (customer == 0) {
        p = problem->getSatellite(solution.getE2Routes()[route].departureSatellite);
    }
    else {
        p = problem->getClient(solution.getE2Routes()[route].tour[customer - 1]);
    }

    if (customer == solution.getE2Routes()[route].tour.size() - 1) {
        q = problem->getSatellite(solution.getE2Routes()[route].departureSatellite);
    }
    else {
        q = problem->getClient(solution.getE2Routes()[route].tour[customer + 1]);
    }
    return problem->getDistance(p, q) - (problem->getDistance(p, c) + problem->getDistance(c, q));
}

// Todo Implement Local Search Step for IDCH
bool IDCH::doLocalSearch(Solution &solution) {
    return false;
}

