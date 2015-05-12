#include "lm/interpolate/backoff_reunification.hh"
#include "lm/builder/model_buffer.hh"
#include "lm/builder/ngram.hh"
#include "lm/builder/sort.hh"

#include <cassert>

namespace lm {
namespace interpolate {

class MergeWorker {
public:
  MergeWorker(std::size_t order, const util::stream::ChainPosition &prob_pos,
              const util::stream::ChainPosition &boff_pos)
      : order_(order), prob_pos_(prob_pos), boff_pos_(boff_pos) {
    // nothing
  }

  void Run(const util::stream::ChainPosition &position) {
    lm::builder::NGramStream stream(position);

    for (util::stream::Stream prob_input(prob_pos_), boff_input(boff_pos_);
         prob_input && boff_input; ++prob_input, ++boff_input, ++stream) {

      const WordIndex *start
          = reinterpret_cast<const WordIndex *>(prob_input.Get());
      const WordIndex *end = start + order_;
      std::copy(start, end, stream->begin());

      stream->Value().complete.prob = *reinterpret_cast<const float *>(end);
      stream->Value().complete.backoff
          = *reinterpret_cast<float *>(boff_input.Get());
    }
    stream.Poison();
  }

private:
  std::size_t order_;
  util::stream::ChainPosition prob_pos_;
  util::stream::ChainPosition boff_pos_;
};

// TODO: Figure out why I *have* to have the output chains here instead of
// ChainPositions
void ReunifyBackoff(util::stream::ChainPositions &prob_pos,
                    util::stream::ChainPositions &boff_pos,
                    util::stream::Chains &output_chains) {
  assert(prob_pos.size() == boff_pos.size());

  for (size_t i = 0; i < prob_pos.size(); ++i)
    output_chains[i] >> MergeWorker(i + 1, prob_pos[i], boff_pos[i]);
}
}
}
