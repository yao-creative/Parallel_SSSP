#include "deltastep_seq.h"

DeltaStepSequential::DeltaStepSequential(const Graph& graph, const int source, const bool is_verbose)
{


	graph_ = graph;
	source_ = source;
	delta_ = 1 / (static_cast<double>(graph_.getMaxDegree()) + 1);
	is_verbose_ = is_verbose;

	// Initialize the graph
	graph_.computeDegrees();
	graph_.createAdjList();
	graph_.computeMaxDegree();

	// Initialize the algorithm
	dist_ = std::vector<double>(graph_.getGraphNbVertices(), std::numeric_limits<double>::infinity());
	pred_ = std::vector<int>(graph_.getGraphNbVertices(), -1);

	light_edges_ = std::vector<std::vector<int>>(graph_.getGraphNbVertices(), std::vector<int>());
	heavy_edges_ = std::vector<std::vector<int>>(graph_.getGraphNbVertices(), std::vector<int>());

	/*adj_matrix_ = graph_.getAdjMatrix();*/
	this->compute_light_and_heavy_edges();

	const int bucket_size = static_cast<int> (graph_.getGraphNbVertices() / delta_) + 1;
	buckets_.resize(bucket_size);

	if (is_verbose_)
		std::cout << "Bucket size: " << bucket_size << std::endl;

	for (int i = 0; i < graph_.getGraphNbVertices(); i++)
	{
		if (i != source_)
			buckets_[bucket_size - 1].insert(i);

	}

	// Set the initial bucket
	buckets_[0].insert(source_);
	dist_[source_] = 0;
	pred_[source_] = source_;

	if (is_verbose_)
	{
		// Display information before the algorithm starts
		this->print_light_and_heavy_edges();
		graph_.print_graph_info();
		this->print_all_buckets();
	}
}

void DeltaStepSequential::compute_light_and_heavy_edges()
{
	for (const Edge& edge : graph_.getEdges())
	{
		if (edge.get_weight() <= delta_)
		{
			light_edges_[edge.get_from()].push_back(edge.get_to());
		}
		else
		{
			heavy_edges_[edge.get_from()].push_back(edge.get_to());
		}
	}
	//for (int i = 0; i < graph_.getGraphNbVertices(); i++)
	//{
	//	for (int j = 0; j < graph_.getGraphNbVertices(); j++)
	//	{
	//		const double edge_weight = graph_.getEdgeWeight(i, j);
	//		if (edge_weight != 0.)
	//		{
	//			if (edge_weight <= delta_)
	//			{
	//				light_edges_[i].push_back(j);
	//			}
	//			else
	//			{
	//				heavy_edges_[i].push_back(j);
	//			}
	//		}
	//		/*if (adj_matrix_[i][j] != 0.)
	//		{
	//			if (adj_matrix_[i][j] <= delta_)
	//			{
	//				light_edges_[i].push_back(j);
	//			}
	//			else
	//			{
	//				heavy_edges_[i].push_back(j);
	//			}
	//		}*/
	//	}
	//}
}

void DeltaStepSequential::find_bucket_requests(const std::set<int>& bucket, std::vector<Edge>* light_requests, std::vector<Edge>* heavy_requests)
{
	for (const int vertex_id : bucket)
	{
		buckets_[bucket_counter_].erase(vertex_id);
		if (is_verbose_)
		{
			std::cout << "Erased " << vertex_id << " from bucket " << bucket_counter_ << std::endl;
			this->print_bucket(bucket_counter_);
		}

		// Add light requests
		for (const int l_edge_vertex_id : light_edges_[vertex_id])
		{

			light_requests->emplace_back(
				vertex_id,
				l_edge_vertex_id,
				graph_.getEdgeWeight(vertex_id, l_edge_vertex_id)
			);

		}

		// Add heavy requests
		for (const int h_edge_vertex_id : heavy_edges_[vertex_id])
		{

			heavy_requests->emplace_back(
				vertex_id,
				h_edge_vertex_id,
				graph_.getEdgeWeight(vertex_id, h_edge_vertex_id)
			);
		}
	}
}

void DeltaStepSequential::print_light_and_heavy_edges()
{
	std::cout << "Light edges: " << std::endl;
	for (int i = 0; i < graph_.getGraphNbVertices(); i++)
	{
		std::cout << "Vertex " << i << ": ";
		for (const int light_edge : light_edges_[i])
		{
			std::cout << light_edge << " ";
		}
		std::cout << std::endl;
	}
	std::cout << "Heavy edges: " << std::endl;
	for (int i = 0; i < graph_.getGraphNbVertices(); i++)
	{
		std::cout << "Vertex " << i << ": ";
		for (const int heavy_edge : heavy_edges_[i])
		{
			std::cout << heavy_edge << " ";
		}
		std::cout << std::endl;
	}
}

void DeltaStepSequential::print_all_buckets() const
{
	for (size_t bucket_id = 0; bucket_id < buckets_.size(); bucket_id++)
	{
		this->print_bucket(bucket_id);
	}
}

void DeltaStepSequential::print_bucket(const size_t bucket_id) const
{
	std::cout << "Bucket [" << bucket_id << "], size " << buckets_[bucket_id].size() << ": ";
	for (const int bucket_item : buckets_[bucket_id])
	{
		std::cout << bucket_item << " ";
	}
	std::cout << std::endl;
}
void DeltaStepSequential::relax(Edge selected_edge) {
	const int from_vertex = selected_edge.get_from();
	const int to_vertex = selected_edge.get_to();
	const double edge_weight = selected_edge.get_weight();
	const double tentative_dist = dist_[from_vertex] + edge_weight;
	if (tentative_dist < dist_[to_vertex]) {

		const int i = static_cast<int> (std::floor(dist_[to_vertex] / delta_));
		const int j = static_cast<int> (std::floor(tentative_dist / delta_));
		//if (j < i) {
		// Check if i is out of bounds
		if (i < buckets_.size() && i >= 0)
		{
			buckets_[i].erase(to_vertex);
		}
		if (j < buckets_.size() && j >= 0)
		{
			buckets_[j].insert(to_vertex);
		}


		//}
		dist_[to_vertex] = tentative_dist;
		pred_[to_vertex] = from_vertex;
	}
}

void DeltaStepSequential::resolve_requests(std::vector<Edge>* requests)
{
	for (const Edge& request : *requests)
	{
		this->relax(request);
	}
}

void DeltaStepSequential::solve() {
	while (bucket_counter_ < buckets_.size())
	{
		std::set<int> current_bucket = buckets_[bucket_counter_];
		while (!current_bucket.empty()) {
			std::set<int> current_bucket_update;

			for (const int vertex_id : current_bucket) {
				const std::set<int> neighbors = graph_.get_vertex_neighbors(vertex_id);
				for (const int neighbor_vertex : neighbors) {
					const int i = static_cast<int>(std::floor(dist_[neighbor_vertex] / delta_));
					dist_[neighbor_vertex] = std::min(dist_[vertex_id] + graph_.getEdgeWeight(vertex_id, neighbor_vertex), dist_[neighbor_vertex]);
					const int j = static_cast<int> (std::floor(dist_[neighbor_vertex] / delta_));
					if (j < i) {
						buckets_[i].erase(neighbor_vertex);
						buckets_[j].insert(neighbor_vertex);
						if (j == bucket_counter_) {
							current_bucket_update.insert(neighbor_vertex);
						}
					}
				}
			}
			current_bucket = current_bucket_update;
		}
		bucket_counter_++;
		while (bucket_counter_ < graph_.getGraphNbVertices() && buckets_[bucket_counter_].empty()) {
			bucket_counter_++;
		}
	}
}
void DeltaStepSequential::solve_light_heavy() {
	while (bucket_counter_ < buckets_.size())
	{
		std::vector<Edge> light_requests, heavy_requests;
		std::set<int> current_bucket = buckets_[bucket_counter_];
		while (!current_bucket.empty()) {

			//Find heavy and light requests in the current bucket
			find_bucket_requests(current_bucket, &light_requests, &heavy_requests);

			//Resolve light requests
			resolve_requests(&light_requests);
			light_requests.clear();

			// Update current bucket
			current_bucket = buckets_[bucket_counter_];
		}

		//Resolve Heavy requests
		resolve_requests(&heavy_requests);
		heavy_requests.clear();

		bucket_counter_++;

		while (bucket_counter_ < buckets_.size() && buckets_[bucket_counter_].empty()) {
			bucket_counter_++;
		}
	}
}

void DeltaStepSequential::print_solution()
{
	std::cout << "Solution: " << std::endl;
	for (int i = 0; i < graph_.getGraphNbVertices(); i++)
	{
		std::cout << "Vertex " << i << ": " << dist_[i] << std::endl;
	}
}

