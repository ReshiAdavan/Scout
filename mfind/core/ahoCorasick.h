#ifndef AHOCORASICK_H
#define AHOCORASICK_H

#include <unordered_map>
#include <vector>
#include <queue>
#include <string>
#include <functional>

class AhoCorasick {
private:
    struct Node {
        std::unordered_map<char, Node*> children;
        Node* fail = nullptr;
        std::vector<std::string> output;
    };

    Node* root;
    std::vector<std::string> insertedKeywords;
    size_t maxPatternLength = 0;

public:
    AhoCorasick() {
        root = new Node();
    }

    ~AhoCorasick() {
        std::function<void(Node*)> cleanup = [&](Node* node) {
            for (auto& [_, child] : node->children) {
                cleanup(child);
            }
            delete node;
        };
        cleanup(root);
    }

    void addKeyword(const std::string& keyword) {
        Node* node = root;
        for (char ch : keyword) {
            if (!node->children.count(ch)) {
                node->children[ch] = new Node();
            }
            node = node->children[ch];
        }
        node->output.push_back(keyword);
        insertedKeywords.push_back(keyword);
        maxPatternLength = std::max(maxPatternLength, keyword.size());
    }

    const std::vector<std::string>& getKeywords() const {
        return insertedKeywords;
    }

    size_t getMaxPatternLength() const {
        return maxPatternLength;
    }

    // Failure links
    void build() {
        std::queue<Node*> q;
        root->fail = root;

        // Level 1 fail links
        for (auto& [ch, node] : root->children) {
            node->fail = root;
            q.push(node);
        }

        // BFS trie
        while (!q.empty()) {
            Node* current = q.front(); q.pop();

            for (auto& [ch, child] : current->children) {
                Node* fallback = current->fail;
                while (fallback != root && !fallback->children.count(ch)) {
                    fallback = fallback->fail;
                }

                if (fallback->children.count(ch) && fallback->children[ch] != child) {
                    child->fail = fallback->children[ch];
                } else {
                    child->fail = root;
                }

                // Merge output from fail link
                for (const std::string& kw : child->fail->output) {
                    child->output.push_back(kw);
                }

                q.push(child);
            }
        }
    }

    // Search, return (position, matched word)
    std::vector<std::pair<size_t, std::string>> search(const std::string& text) const {
        std::vector<std::pair<size_t, std::string>> results;
        Node* node = root;

        for (size_t i = 0; i < text.size(); ++i) {
            char ch = text[i];

            while (node != root && !node->children.count(ch)) {
                node = node->fail;
            }

            if (node->children.count(ch)) {
                node = node->children[ch];
            }

            for (const std::string& match : node->output) {
                results.emplace_back(i + 1 - match.size(), match);
            }
        }

        return results;
    }
};

#endif
