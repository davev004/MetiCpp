#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "fen.hpp"
#include "perft.hpp"
#include "zobrist.hpp"
#include "uci.hpp"

namespace {
    void require(bool condition, const char* label) {
        if (!condition) {
            std::fprintf(stderr, "FAILED: %s\n", label);
            std::abort();
        }
    }

    void assert_same_position(const Board& left, const Board& right) {
        require(std::memcmp(left.bitboards, right.bitboards, sizeof(left.bitboards)) == 0, "bitboards");
        require(std::memcmp(left.mailbox, right.mailbox, sizeof(left.mailbox)) == 0, "mailbox");
        require(std::memcmp(left.occupancy, right.occupancy, sizeof(left.occupancy)) == 0, "occupancy");
        require(std::memcmp(&left.state, &right.state, sizeof(left.state)) == 0, "state");
        require(left.ply == right.ply, "ply");
    }

    void test_position(const char* fen, uint64_t depth_one, uint64_t depth_two, uint64_t depth_three = 0) {
        Board board;
        require(FEN::parse(board, fen), "fen");
        Board before = board;
        uint64_t actual_one = Perft::run(board, 1);
        require(actual_one == depth_one, "depth one");
        assert_same_position(board, before);
        uint64_t actual_two = Perft::run(board, 2);
        require(actual_two == depth_two, "depth two");
        assert_same_position(board, before);
        if (depth_three != 0) {
            uint64_t actual_three = Perft::run(board, 3);
            require(actual_three == depth_three, "depth three");
            assert_same_position(board, before);
        }
    }

    Meti::Move find_move(const Board& board, int from, int to, Meti::MoveFlag flag) {
        Meti::MoveList moves;
        MoveGen::generate(board, moves);
        for (int i = 0; i < moves.count; ++i) {
            Meti::Move move = moves.moves[i];
            if (Meti::get_from(move) == from && Meti::get_to(move) == to && Meti::get_flag(move) == flag) {
                return move;
            }
        }
        return 0;
    }

    void test_special_moves() {
        Board board;
        require(FEN::parse(board, "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1"), "en passant fen");
        Meti::Move en_passant = find_move(board, 36, 43, Meti::MOVE_CAPTURE);
        require(en_passant != 0, "en passant generated");
        Board before = board;
        require(is_castling_legal(board, en_passant) && make_move(board, en_passant), "en passant make");
        require(board.mailbox[43] == W_PAWN && board.mailbox[35] == PIECE_NONE, "en passant state");
        unmake_move(board, en_passant);
        assert_same_position(board, before);

        require(FEN::parse(board, "4k3/P7/8/8/8/8/8/4K3 w - - 0 1"), "promotion fen");
        Meti::MoveList promotions;
        MoveGen::generate(board, promotions);
        int promotion_count = 0;
        for (int i = 0; i < promotions.count; ++i) {
            if (Meti::get_from(promotions.moves[i]) == 48 && Meti::get_to(promotions.moves[i]) == 56) {
                ++promotion_count;
            }
        }
        require(promotion_count == 4, "promotion moves");

        require(FEN::parse(board, "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"), "castling fen");
        require(find_move(board, 4, 6, Meti::MOVE_CASTLING) != 0, "white king castle generated");
        require(find_move(board, 4, 2, Meti::MOVE_CASTLING) != 0, "white queen castle generated");
        require(!FEN::parse(board, "r3k2r/8/8/8/8/8/8/4K2R w KQkq - 0 1"), "missing rook rejected");
    }

    void test_uci_move_validation() {
        Board board;
        require(FEN::parse(board, "k7/4P3/8/8/8/8/8/4K3 w - - 0 1"), "uci promotion fen");
        require(UCI::parse_move(board, "e7e8q") != 0, "uci promotion accepted");
        require(UCI::parse_move(board, "e7e8x") == 0, "uci invalid promotion rejected");
        require(UCI::parse_move(board, "e7e9") == 0, "uci invalid coordinate rejected");
        require(UCI::parse_move(board, "e7e8") == 0, "uci missing promotion rejected");
    }
}

int main() {
    Zobrist::init();
    test_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 20, 400, 8902);
    test_special_moves();
    test_uci_move_validation();

    Board board;
    require(!FEN::parse(board, "8/8/8/8/8/8/8/8 w - - 0 1"), "missing kings");
    require(!FEN::parse(board, "8/8/8/8/8/8/8/7K w - e9 0 1"), "invalid ep");
    require(!FEN::parse(board, "r3k2r/8/8/8/8/8/8/4K3 w K - 0 1"), "invalid castle");
    return 0;
}
